#ifndef I_CAMERA_BASE
#define I_CAMERA_BASE

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "t_camera_attrib.h"

#include <QImage>

using namespace std;

class i_vi_camera_base {

public:
    enum e_camsta {

        CAMSTA_UNKNOWN = 0,
        CAMSTA_PREPARED,
        CAMSTA_INPROC,
        CAMSTA_ERROR
    } sta;

    /*! porpodrovane formaty s kterymi pracuje na vstupu a na ktere umi konvertovat
     */
    enum e_camformat {

      CAMF_UNDEF = 0,
      CAMF_8bMONO = 1,
      CAMF_32bRGB
    };

    /*! tyka se metody exposition()
     */
    enum e_camvalue {

        CAMVAL_UNDEF = 0,
        CAMVAL_ABS = 1,
        CAMVAL_AUTO_TOLERANCE,
        CAMVAL_AUTO_TIMEOUT
    };

    struct t_campic_info {

        unsigned h;
        unsigned w;
        e_camformat format;
    };

    struct t_campic {

        t_campic_info t;
        unsigned size;
        void    *p;
    };

protected:
    uint32_t conv_elapsed;
    uint32_t snap_elapsed;
    t_vi_camera_attrib par;     //camera attributes

    /*! \brief interface for taking pictures */
    virtual int isnap(void *img, unsigned free, t_campic_info *info = NULL) = 0;

    /*! \brief setup collection io, read for empty/default val */
    QVariant setup(QString &name, QVariant v = QVariant()){

        t_setup_entry val;
        if(0 == par.ask(name, &val))  //get the config item first
            return QVariant(); //parametr of name do not exists

        if(v.isValid()){

            val.set(v.toJsonValue());  //update actual value (with all restriction applied)
            par.replace(name, val); //writeback
        }

        return val.get();
    }

    int convertf(t_campic &inp, t_campic &out){

        int ret = 0;

        clock_t start = clock();

        uint8_t *po = (uint8_t *)out.p;
        uint8_t *pi = (uint8_t *)inp.p;

        //jak by to melo dopadnout + size nechavame na zadane hodntoe
        //skutecna velikost se vraci jako navratova hodnota
        out.t.w = inp.t.w;  //resizing se neprovadi - todo? k zamysleni
        out.t.h = inp.t.h;
        out.t.format = CAMF_UNDEF;

        QString f = par["pic-format"].get().toString();  //pozadovany format
        if((0 == f.compare("8bMONO")) && (inp.t.format == CAMF_32bRGB)){

            //pres Qt by to vypadalo nejak tak
            //dest = src.convertToFormat(QImage::Format_Grayscale8); je az v Qt5.6

            if((int)out.size < (ret = (inp.t.h * inp.t.w))){ //nemame dostatecny prostor

                ret = -ret;
            } else {

                out.t.format = CAMF_8bMONO;
                for(unsigned x=0; x < inp.t.w; x++)
                    for(unsigned y=0; y < inp.t.h; y++){

                        //itu recomandation Red * 0.3 + Green * 0.59 + Blue * 0.11
                        uint32_t acc = 30 * pi[0] + 59 * pi[1] + 11 * pi[2];  //simple stupid - bez vazeni
                        *po++ = acc / 100;
                        pi += 4; //alfa kanal nezajima
                    }
            }

        } else if((0 == f.compare("32bRGB")) && (inp.t.format == CAMF_8bMONO)){


            //QImage dest = src.convertToFormat(QImage::Format_RGB32); - az budu mit Qt 5.6

            if((int)out.size < (ret = 4 * (inp.t.h * inp.t.w))){ //nemame dostatecny prostor

                ret = -ret;
            } else {

                out.t.format = CAMF_32bRGB;
                for(unsigned x=0; x < inp.t.w; x++)
                    for(unsigned y=0; y < inp.t.h; y++){

                        *po++ = *pi;
                        *po++ = *pi;
                        *po++ = *pi++;
                        *po++ = 0; //alfa kanal nezajima
                    }
            }

        } else { //beze zmeny - jen to prekopiruju

            if((int)out.size < (ret = inp.t.h * inp.t.w)){ //nemame dostatecny prostor

                ret = -ret;
            } else {

                out.t.format = inp.t.format;
                memcpy(po, pi, inp.size);
            }
        }

#ifdef QT_DEBUG
        QImage img;
        if(out.t.format == CAMF_8bMONO){

            img = QImage((uchar *)out.p, out.t.w, out.t.h, QImage::Format_Indexed8);
            QVector<QRgb> gr_table;
            for(int i = 0; i < 256; i++) gr_table.push_back(qRgb(i,i,i));
            img.setColorTable(gr_table);
        } else if(out.t.format == CAMF_32bRGB){

            img = QImage((uchar *)out.p, out.t.w, out.t.h, QImage::Format_RGB32);
        }

        img.save("cam-conversion.bmp");
#endif //QT_DEBUG

        clock_t stop = clock();
        conv_elapsed = ((stop - start) * 1000.0) / CLOCKS_PER_SEC;
        qDebug() << QString("img-conversion takes %1ms").arg(conv_elapsed);
        return ret;  //povedlo se
    }


public:
    /*! \brief before fist grab / for reset do original state */
    virtual int init(){

        return 0;
    }

    /*! \brief exposition control */
    virtual int64_t exposition(int64_t time, e_camvalue act = CAMVAL_UNDEF){

        Q_UNUSED(time)
        Q_UNUSED(act)
        return 0;
    }

    /*! \brief picture acquisition according to current t_vi_camera_attrib */
    int snap(void *img, unsigned free, t_campic_info *info = NULL){

        clock_t start = clock();
        int ret = isnap(img, free, info);
        clock_t stop = clock();
        snap_elapsed = ((stop - start) * 1000.0) / CLOCKS_PER_SEC;
        qDebug() << QString("img-snapshot takes %1ms").arg(snap_elapsed);
        return ret;
    }


    /*! \brief return camera status */
    virtual e_camsta state(){
        return sta;
    }


    /*! \brief read attribute 'name' */
    QVariant get(QString &name){

        return setup(name);
    }

    /*! \brief write attribute 'name' */
    QVariant set(QString &name, QVariant v){

        return setup(name, v);
    }


    i_vi_camera_base(){

        sta = CAMSTA_UNKNOWN;
    }

    i_vi_camera_base(const QString &path):
        par(path){

        sta = CAMSTA_UNKNOWN;
    }

    virtual ~i_vi_camera_base(){;}
};

#endif // I_CAMERA_BASE

