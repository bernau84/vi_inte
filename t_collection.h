#ifndef I_COLLECTION
#define I_COLLECTION

#include <QObject>
#include <QEventLoop>
#include <QElapsedTimer>

#include <stdio.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "cinterface/i_comm_generic.h"
#include "cameras/basler/t_vi_camera_basler_usb.h"
#include "cameras/basler/t_vi_camera_basler_gige.h"
#include "cameras/offline/t_vi_camera_offline_file.h"
#include "processing/t_vi_proc_roi_colortransf.h"
#include "processing/t_vi_proc_threshold_cont.h"
#include "processing/t_vi_proc_roll_approx.h"
#include "processing/t_vi_proc_sub_background.h"
#include "processing/t_vi_proc_statistic.h"
#include "processing/t_vi_proc_rectification.h"

#include "t_vi_setup.h"
#include "t_vi_specification.h"
#include "t_record_storage.h"

//global definition of txt remote orders
extern const char *ords[];

class i_collection : public QObject {

    Q_OBJECT

private:

    QElapsedTimer etimer;
    QElapsedTimer ptimer;

    QImage snapshot;
    QString path;
    QString log;

    t_results props;
    t_collection par;
    bool abort;
    uint32_t error_mask;  //suma flagu e_vi_plc_pc_errors

#ifdef USE_USB
    t_vi_camera_basler_usb cam_device;
#elif defined USE_GIGE
    t_vi_camera_basler_gige cam_device;
#endif //USE_XXX

    t_vi_camera_offline_file cam_simul;

    t_vi_proc_rectify   re;
    t_vi_proc_colortransf ct;
    t_vi_proc_threshold th;
    t_vi_proc_roll_approx ms;
    t_vi_proc_sub_backgr bc;
    t_vi_proc_statistic st;

    t_comm_tcp_rollidn iface;

    t_record_storage store;

    float ref_luminance;   //referencni jas odecteny po ustaleni atoexpozice - prvni po startu programu
    float last_luminance;   //referencni jas odecteny po ustaleni atoexpozice - poslendi mereni backgroundu
    float act_luminance;   //jas posledniho snimku

    uint64_t exposition;  //aktualni hodnota expozice

    //constructor helpers
    QJsonObject __from_file(){

        // default config
        QFile f_def(path);  //from resources
        if(f_def.open(QIODevice::ReadOnly | QIODevice::Text)){

            QByteArray f_data = f_def.read(64000);

            QJsonDocument js_doc = QJsonDocument::fromJson(f_data);
            if(!js_doc.isEmpty()){

                //qDebug() << js_doc.toJson();
                return js_doc.object();
            }
        }

        return QJsonObject();
    }

    //constructor helpers
    bool __to_file(){

        // default config
        QFile f_def(path);  //from resources
        if(f_def.open(QIODevice::WriteOnly | QIODevice::Text)){

            //if(!par.isEmpty()){

                QJsonDocument js_doc(par);
                f_def.write(js_doc.toJson());
                return true;
            //}
        }

        return false;
    }

protected:
    void reply(unsigned ord){

        switch(ord){

            case VI_PLC_PC_TRIGGER_ACK: //meas
                log += QString("-->tx: TRIGGER\r\n");
            break;
            case VI_PLC_PC_READY:
                log += QString("-->tx: READY\r\n");
            break;
            case VI_PLC_PC_ERROR:
                log += QString("-->tx: ERROR\r\n");
            break;
            case VI_PLC_PC_BACKGROUND_ACK:
                log += QString("-->tx: BACKGROUND-ACK\r\n");
            break;
            case VI_PLC_PC_RESULT:
                log += QString("-->tx: RESULT\r\n");
            break;
        }
    }

    void receive(unsigned ord, QByteArray raw){

        switch(ord){

            case VI_PLC_PC_TRIGGER: //meas
                log += QString("<--rx: TRIGGER\r\n");
            break;
            case VI_PLC_PC_ABORT:
                log += QString("<--rx: ABORT\r\n");
            break;
            case VI_PLC_PC_READY:
                log += QString("<--rx: READY\r\n");
            break;
            case VI_PLC_PC_ERROR:
                log += QString("<--rx: ERROR\r\n");
            break;
            case VI_PLC_PC_BACKGROUND:
                log += QString("<--rx: BACKGROUND\r\n");
            break;
            case VI_PLC_PC_CALIBRATE:
                log += QString("<--rx: CALIBRATE\r\n");
            break;
        }
    }


    void __init_measurement_res() = 0;

    void __eval_measurement_res() = 0;

    void __serialize_measurement_res(void *to, unsigned reserved) = 0;

    void __deserialize_measurement_res(void *from, unsigned reserved) = 0;

public slots:

    virtual int on_done(int res, void *img){

        res = res;
        img = img;
        return 0;
    }

    //slot co se zavola s prijmem povelu od plc
    virtual int on_order(unsigned ord, QByteArray raw){

        log.clear();

        etimer.start();

        switch(ord){

            case VI_PLC_PC_TRIGGER: //meas
            {
                store.increment();

                error_mask = VI_ERR_OK;

                //potvrdime prijem
                reply(VI_PLC_PC_TRIGGER_ACK); //tx ack to plc

                on_meas();
                reply(VI_PLC_PC_RESULT);  //tx results to plc
            }
            break;
            case VI_PLC_PC_ABORT:
                on_abort(); //nastavi preruseni a ceka na jeho vyhodnoceni
                //a prekontrolujem jak na tom sme
            break;
            case VI_PLC_PC_READY:
            {
                if(on_ready()){
                    //potvrdime prijem
                    reply(VI_PLC_PC_READY);
                } else {
                    //nejsme operabilni
                    reply(VI_PLC_PC_ERROR);
                }
            }
            break;
            case VI_PLC_PC_BACKGROUND:
            {
                error_mask = VI_ERR_OK;
                if(on_background()){
                    //odvysilame vysledek
                    reply(VI_PLC_PC_BACKGROUND_ACK);
                }
            }
            break;
            case VI_PLC_PC_CALIBRATE:
            {
                store.increment();

                error_mask = VI_ERR_OK;

                on_calibration();

                reply(VI_PLC_PC_CALIBRATE_ACK);
            }
            break;
        }

        log += QString("command time %1ms\r\n").arg(etimer.elapsed());
        store.append(log);

        return 0;
    }

    //odpaleni snimani analyza a odreportovani vysledku
    //volame zatim rucne
    int on_trigger(bool background = false){

        const int img_reserved = 3000 * 2000;
        uint8_t *img = (uint8_t *) new uint8_t[img_reserved];
        if(!img){

            error_mask |= VI_ERR_CAM_MEMORY;
            log += QString("cam-error: alocation failed!");
            return 0;
        }

        i_vi_camera_base::t_campic_info info;

        //acquisition
        int pisize = 0;
        for(int rep = 0; pisize <= 0; rep++){

            if(cam_device.sta == i_vi_camera_base::CAMSTA_PREPARED){

                pisize = cam_device.snap(img, img_reserved, &info);
            } else {

                error_mask |= VI_ERR_CAM_NOTFOUND;
                pisize = cam_simul.snap(img, img_reserved, &info);
            }

            if((rep >= 5) || abort){

                error_mask |= VI_ERR_CAM_TIMEOUT;
                switch(pisize){

                    case 0:     error_mask |= VI_ERR_CAM_SNAPERR;   break;
                    case -101:  error_mask |= VI_ERR_CAM_BADPICT;   break;
                    case -102:  error_mask |= VI_ERR_CAM_EXCEPTION; break;
                }

                log += QString("cam-error: timeout / abort");
                abort = false;
                return 0;
            }

            QEventLoop loop;  //process pottential abort
            loop.processEvents();
        }

        if(info.w * info.h <= 0){

            error_mask |= VI_ERR_CAM_BADPICT;
            log += QString("cam-error: bad picture");
            return 0;
        }

        ptimer.start();

        QVector<QRgb> colorTable;
        for (int i = 0; i < 256; i++)
            colorTable.push_back(QColor(i, i, i).rgb());

        snapshot = QImage(img, info.w, info.h, QImage::Format_Indexed8);
        snapshot.setColorTable(colorTable);

        store.insert(snapshot);

        emit present_preview(snapshot, 0, 0);    //vizualizace preview kamery

        //process measurement or save new background
        cv::Mat src(info.h, info.w, CV_8U, img);
        int order = (background) ? t_vi_proc_sub_backgr::SUBBCK_REFRESH : t_vi_proc_sub_backgr::SUBBCK_SUBSTRACT;
        re.proc(order, &src);

        //calc average brightness
        st.proc(t_vi_proc_statistic::STATISTIC_BRIGHTNESS, &src);
        act_luminance = st.out.at<float>(0); //extract luminance from output matrix

        if(img) delete[] img;

        log += QString("analysis time %1ms\r\n").arg(ptimer.elapsed());
        return 1;
    }

    //obsahuje snimani obrazu a analyzu
    int on_meas(void){

        __init_measurement_res();

        int res = on_trigger();
        if(!res) res = on_trigger(); //opakujem 2x pokud se mereni nepovede
        if(res){

            __eval_measurement_res();  //prepocet na mm

            QVector<QRgb> colorTable;
            for (int i = 0; i < 256; i++)
                colorTable.push_back(QColor(i, i, i).rgb());

            //vizualizace mereni
            QImage output(ms.loc.data, ms.loc.cols, ms.loc.rows, QImage::Format_Indexed8);
            output.setColorTable(colorTable);
            emit present_meas(output, mm_length, mm_diameter);
        } else {

            QImage output(":/error-498-fix.gif");
            emit present_meas(output, mm_length, mm_diameter);
        }

        return res;
    }

    int on_abort(){

        abort = true;

        /*! \todo - qloop a cekame na vyhodnoceni (smazani abortu) */
        return 1;
    }

    int on_ready(){

//        //zafixujeme nastaveni expozice a ulozime si referencni hodnotu jasu
//        if(ref_luminance < 0){

            error_mask = VI_ERR_OK;
            if(cam_device.sta != i_vi_camera_base::CAMSTA_PREPARED){

                error_mask |= VI_ERR_CAM_NOTFOUND;
                return 0;
            }

//FIXME: zaremovano protoze zpusobovalo pady - takhle zustava ref-luminance zaporna a nic se proto
            //se expozici za behu nedeje

//            else if(cam_device.exposure(100, i_vi_camera_base::CAMVAL_AUTO_TOLERANCE)){ //100us tolerance to settling exposure

//               on_trigger(true); //true == background mode
//               if(error_mask == VI_ERR_OK){

//                  last_luminance = ref_luminance = act_luminance;
//                  return 1;
//               }
//            } else {

//               error_mask |= VI_ERR_CAM_EXPOSITION;  //autoexpozice failovala
//               return 0;
//            }
//        }

        return 1;
    }

    /*! vychazime z referenci expoxice a jasu sceny po startu
     * pokud se jas sceny lisi, korigujemve stejnem pomeru i expozici
     * !referencni expozici ani jas nemenime !
     */
    int on_background(){

        bool mode = true; //false - bez noveho pozadi

        if((ref_luminance > 0) && cam_device.sta == i_vi_camera_base::CAMSTA_PREPARED){

            exposition = cam_device.exposure(0, i_vi_camera_base::CAMVAL_UNDEF);

            float dif_luminance = ref_luminance / last_luminance;
            if(dif_luminance > 1.05){ //moc temne

                mode = true;    //budem chti novy snime pozadi
                //pomerove zvednem expozici nahoru
                int64_t nexpo = exposition * dif_luminance;
                nexpo = cam_device.exposure(nexpo, i_vi_camera_base::CAMVAL_ABS);
            } else if(dif_luminance < 0.95){  //moc svtele

                mode = true;    //budem chtit novy snimek pozadi
                //pomerove snizime expozici dolu
                int64_t nexpo = exposition * dif_luminance;
                nexpo = cam_device.exposure(nexpo, i_vi_camera_base::CAMVAL_ABS);
            }
        }

        if(on_trigger(mode)){ //true == background mode

            QVector<QRgb> colorTable;
            for (int i = 0; i < 256; i++)
                colorTable.push_back(QColor(i, i, i).rgb());

            //alespon rekrifikovany obrazek
            QImage output(re.out.data, re.out.cols, re.out.rows, QImage::Format_Indexed8);
            output.setColorTable(colorTable);
            emit present_meas(output, 0, 0);
        } else {

            QImage output(":/error-498-fix.gif");
            emit present_meas(output, 0, 0);
        }

        if(error_mask == VI_ERR_OK){

            last_luminance = act_luminance;
            return 1;
        }

        return 0;
    }

    int on_calibration(){

        last_luminance = ref_luminance = -1;  //indikuje ze nebylo dosud provedeno nastaeni expozice
        return on_trigger();
    }

signals:
    void process_next(int p1, void *p2);  //pro vstupovani mezi jednotlive analyzy
    void present_meas(QImage &im, double length, double diameter);  //vizualizace mereni
    void present_preview(QImage &im, double length, double diameter);    //vizualizace preview kamery

public slots:

    int intercept(int p1, void *p2){

        /*! place some data manipulation between
         * two t_vi_proc_* stages
         */

        emit process_next(p1, p2);
        return 1;
    }

public:

    int initialize(){

        cam_device.init();
        cam_simul.init();
        return 1;
    }

    t_collection(QString &js_config, QObject *parent = NULL):
        QObject(parent),
        path(js_config),
        par(__from_file()),
        cam_device(path),
        cam_simul(path),
        abort(false),
        re(path),
        ct(path),
        th(path),
        ms(path),
        bc(path),
        iface(par["tcp-server-port"].get().toInt(), this),
        store(QDir::currentPath() + "/storage")
    {
        //zretezeni analyz
        QObject::connect(&re, SIGNAL(next(int, void *)), &bc, SLOT(proc(int, void *)));
        QObject::connect(&bc, SIGNAL(next(int, void *)), &th, SLOT(proc(int, void *)));
        QObject::connect(&th, SIGNAL(next(int, void *)), &ms, SLOT(proc(int, void *)));

        /*! pokud je potreba je mozne mezi analyzy vstoupit slotem intercept(int, void *)
         * tohoto obektu a pomoci toho treba runtime menit parametry
         */

        /*! \todo - navazat vystupem ms na ulozeni vysledku analyzy (obrazek) */

        //z vnejsu vyvolana akce
        QObject::connect(&iface, SIGNAL(order(unsigned, QByteArray)), this, SLOT(on_order(unsigned, QByteArray)));

        if(0 >= (area_min = par["contour_minimal"].get().toDouble()))
            area_min = ERR_MEAS_MINAREA_TH;

        if(0 >= (area_max = par["contour_maximal"].get().toDouble()))
            area_max = ERR_MEAS_MAXAREA_TH;

        last_luminance = ref_luminance = -1;  //indikuje ze nebylo dosud provedeno
    }

    ~t_collection(){

    }
};

#endif // I_COLLECTION

