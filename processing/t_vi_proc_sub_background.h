#ifndef T_VI_PROC_SUB_BACKGROUND
#define T_VI_PROC_SUB_BACKGROUND

#include "i_proc_stage.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <QString>

using namespace cv;
using namespace std;

static const QString proc_sub_backgr_defconfigpath(":/js_config_sub_background.txt");

class t_vi_proc_sub_backgr : public i_proc_stage
{

private:
    int bck_lighter_thresh;
    int bck_darker_thresh;

    QString bpath;

    int bck_avr;
    int bck_num;

    Mat out;
public:

    enum t_vi_proc_sub_backgr_ord {
        SUBBCK_RESET = 0,
        SUBBCK_REFRESH = 1,
        SUBBCK_SUBSTRACT = 2
    };

    t_vi_proc_sub_backgr(const QString &path = proc_sub_backgr_defconfigpath):
        i_proc_stage(path)
    {
        fancy_name = "subst-background(" + fancy_name + ")";
        reload(0);
        qDebug() << "Sub background image / light-threshold / dark-threshold:" << bpath << bck_lighter_thresh << bck_darker_thresh;
    }

    virtual ~t_vi_proc_sub_backgr(){;}

public slots:
    int reload(int p){

        p = p;

        bck_num = 0;

        //pozadi svetlejsi nez L1 je nahrazeno 0
        if(0 >= (bck_lighter_thresh = par["bck-ligher-threshold"].get().toInt()))
            bck_lighter_thresh = 32;

        //popredi svetlejsi nez L2 zustava nezmeneno
        if(0 >= (bck_darker_thresh = par["bck-darker-threshold"].get().toInt()))
            bck_darker_thresh = 16;

        //prumerovani pozadi
        if(0 >= (bck_avr = par["background-avr"].get().toInt()))
            bck_avr = 1;

        //cesta ke snimku pozadi
        bpath = par["background"].get().toString();
        if(bpath.isEmpty()) bpath = QString("back.bmp");

        return 1;
    }

private:
    int iproc(int p1, void *p2){

        //const char *std_bpath = "c:\\Users\\bernau84\\Documents\\sandbox\\roll_idn\\build-processing-Desktop_Qt_5_4_1_MSVC2010_OpenGL_32bit-Debug\\debug\\back.bmp";
        //const char *std_bpath = "c:\\Users\\bernau84\\Pictures\\trima_daybackground\\trn_bck_exp1_1.bmp";

        QByteArray ba_bpath= bpath.toLatin1();
        const char *std_bpath = ba_bpath.data();

        Mat tbck = imread(std_bpath);
        Mat tsrc = *(Mat *)p2;
        int tord = (t_vi_proc_sub_backgr_ord)p1;

        if(tord == SUBBCK_RESET){

            qDebug() << "t_vi_proc_sub_background averaging reset.";
            bck_num = 0;
            return 0;
        } else if(tsrc.empty()){

            qDebug() << "t_vi_proc_sub_background empty input!";
            return 0;  //neni co pocitat
        }

        Mat inp = tsrc; //drive zde byla take converze barev
        Mat bck;

        //sjednoceni formatu - menime format pozadi, pokud nejake je
        //podporuje ale jen jednoduche 1 nebo 3 kanaly
        int desired = tsrc.channels();
        int available = tbck.channels();

        if(!tbck.empty()){

            bck = tbck;
            switch(available){
                case 1:
                    if(desired == 3) cvtColor(tbck, bck, CV_GRAY2RGB);
                    else if(desired == 4) cvtColor(tbck, bck, CV_GRAY2RGBA);
                    else {
                        qDebug() << "t_vi_proc_sub_background input format error(-1) (1,3,4 channels are supported only)!";
                        return 0;
                    }
                break;
                case 3:
                    if(desired == 1) cvtColor(tbck, bck, CV_RGB2GRAY);
                    else if(desired == 4) cvtColor(tbck, bck, CV_RGB2BGRA);
                    else {
                        qDebug() << "t_vi_proc_sub_background input format error(-2)  (1,3,4 channels are supported only)!";
                        return 0;
                    }
                break;
                case 4:
                    if(desired == 1) cvtColor(tbck, bck, CV_RGBA2GRAY);
                    else if(desired == 3) cvtColor(tbck, bck, CV_RGBA2BGR);
                    else {
                        qDebug() << "t_vi_proc_sub_background input format error(-3) (1,3,4 channels are supported only)!";
                        return 0;
                    }
                break;
                default:
                    qDebug() << "t_vi_proc_sub_background background format error (1,3,4 channels are supported only)!";
                    return 0;  //neni s cim pocitat
            }
        }

        switch(tord){

            case SUBBCK_REFRESH:

                //nekdo nam smazal pozdi - zacnem prumerovat znovu
                //nebo byl zadav externi povel nebo to nema smysl protoze prumerovani je vypnute
                if((bck_avr < 2) || (bck_num == 0) || (tbck.empty())){

                    bck = inp;
                } else {

                    //FIXME: nefunguje, svetle plochy postupne saturuji
                    //otazka jestli to budem moct pouzit s promennou ecpozici
                    //stejne ne
                    //bck += (inp - bck) / 2;
                }

                imwrite(std_bpath, bck);
                //imshow("Backround cummulative", bck);
                return ++bck_num;  //konec signal dal nesirime

            default:  //normalni mode

                if(tbck.empty() ||
                   tbck.cols != tsrc.cols ||
                   tbck.rows != tsrc.rows ){

                    elapsed = etimer.elapsed();
                    emit next(0, &tsrc);
                    return 0;
                }

                break;
        }


        out = Mat(inp.rows, inp.cols, (inp.channels() == 1) ? CV_8U : CV_8UC3);

        int L1 = bck_lighter_thresh;  //pozadi svetlejsi nez L1 je nahrazeno 0
        int L2 = bck_darker_thresh;  //popredi svetlejsi nez L2 zustava nezmeneno

        //substract background and saturate
        //cv::absdiff(imp, bck, out);

        for(int y = 0; y < inp.rows; y++){

            for(int x = 0; x < inp.cols; x++)
                for(int r=0; r < inp.channels(); r++){ //pres barvicky

                    uchar c, b;
                    switch(inp.channels()){
                            case 4:
                                c = inp.at<cv::Vec4b>(y,x)[r];  //mereni
                                b = bck.at<cv::Vec4b>(y,x)[r];  //pozadi
                            break;
                            case 3:
                                c = inp.at<cv::Vec3b>(y,x)[r];  //mereni
                                b = bck.at<cv::Vec3b>(y,x)[r];  //pozadi
                            break;
                            case 1:
                                c = inp.at<uchar>(y,x);  //mereni
                                b = bck.at<uchar>(y,x);  //pozadi
                            break;
                    }
                    int d = (int)c - (int)b; //vysledek

                    //baze pravidel pro odecitani pozadi
                    if(d < 0){

                        if(d < L1) d = 0; //velka zmena - pozadi svetlejsi - divne, dame 0
                        else d = -d;  //popredi je nevyznamne - berem absolutni hodnotu rozdilu

                    } else {

                        if((c > L2) && (d > L2)) d = c; //popredi vyznamne a rozdil velky - kasle na nej
                    }

                    //if((d *= 2) > 255) d = 255;

                    switch(out.channels()){
                            case 4: //na vystupo podporujem jen 1 nebo 3 kanalovy obrazek
                            case 3: if(r < 3) out.at<cv::Vec3b>(y,x)[r] = d;  break;
                            case 1: out.at<uchar>(y,x) = d; break;
                    }
                }
        }

        elapsed = etimer.elapsed();
        emit next(0, &out);
        return 1;
    }
};


#endif // T_VI_PROC_SUB_BACKGROUND

