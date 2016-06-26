#ifndef T_INTEVA_TESNENI
#define T_INTEVA_TESNENI

#include <QObject>
#include <QPainter>

#include <stdio.h>
#include <algorithm>
#include "i_collection.h"
#include "t_inteva_specification.h"

#include "processing/t_vi_proc_fitline.h"
#include "processing/t_vi_proc_threshold_cont.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

//to do - vyresit jak uchovat a kruhove ukladat 
//stdout v bashi

class t_inteva_app : public i_collection {
	
	Q_OBJECT

private:

    t_vi_proc_fitline l_fl;
    t_vi_proc_fitline r_fl;
    t_vi_proc_threshold th;

    float perc_lfit;
    float perc_rfit;
    float mm_gap;

    uint32_t meas_count;

    //x ovy prusecik s horizontalou x == h
    float x_crossing(cv::Vec4f &line, float h){

//        "vx" << QString::number(line[0]);
//        "vy" << QString::number(line[1]);
//        "x0" << QString::number(line[2]);
//        "y0" << QString::number(line[3]);
        return line[2] + (h - line[3])*(line[0] / line[1]);
    }

    //preproces - spusten pred snimanim obrazu
    void __init_measurement() {

        perc_lfit = perc_rfit = 0;
        mm_gap = 0.0;
        meas_count = 0;
    }

    //potprocess - zpracovani hodnot z analyz 
    void __eval_measurement() {

        meas_count++;

        //vysetreni primky vuci tolerancnimu poli
        int D = par["toler-zone"].get().toInt();
        int G = par["toler-gap"].get().toInt();
        int X = par["toler-offset"].get().toInt();

        int me_d = r_fl.line[2] - l_fl.line[2];  //stredni vzdalenost primek
        int up_d_r = x_crossing(r_fl.line, 0);
        int up_d_l = x_crossing(l_fl.line, 0);  //horni vzdalenost
        int dw_d_r = x_crossing(r_fl.line, info.h);
        int dw_d_l = x_crossing(l_fl.line, info.h);  //spodni

        int perc_lfit_up = up_d_l - (X - D/2);
        int perc_lfit_dw = dw_d_l - (X - D/2);

        int perc_rfit_up = up_d_r - (X + D/2);
        int perc_rfit_dw = dw_d_r - (X + D/2);

        //pokud jsou oba kraje uvnitr tolerance tak 100% jinak primka vycuhuje v pomeru min / max
        perc_lfit = 100;
        if((perc_lfit_up < 0) && (perc_lfit_dw < 0))
            perc_lfit = 0;
        else if((perc_lfit_up * perc_lfit_dw) < 0)
            perc_lfit *= (min(perc_lfit_up, perc_lfit_dw) / (0.5 + max(perc_lfit_up, perc_lfit_dw)));  //0.5 prevence overflow

        perc_rfit = 100;
        if((perc_rfit_up < 0) && (perc_rfit_dw < 0))
            perc_rfit = 0;
        else if((perc_rfit_up * perc_rfit_dw) < 0)
            perc_rfit *= (min(perc_rfit_up, perc_rfit_dw) / (0.5 + max(perc_rfit_up, perc_rfit_dw)));

        mm_gap = (me_d + (up_d_r - up_d_l) + (dw_d_r - dw_d_l)) / 3;

        if(perc_rfit < 100){

            error_mask &= ~VI_ERR_OK;
            error_mask |= VI_ERR_MEAS1;
        }

        if(perc_lfit < 100){

            error_mask &= ~VI_ERR_OK;
            error_mask |= VI_ERR_MEAS2;
        }

        //prepocet z pix na mm - provadime jem pokud mame zakladni kalibraci
        if(!par.ask("plane-cam-distance-mm") || !par.ask("focal-length-mm"))
            return;

        mm_gap *= par["plane-cam-distance-mm"].get().toInt();
        mm_gap /= par["focal-length-mm"].get().toInt(); //to je stale v pix
        mm_gap *= 2.2e-6; //rozliseni kamery - v mm/pix; vysledek v mm

        if(mm_gap > G){

            error_mask &= ~VI_ERR_OK;
            error_mask |= VI_ERR_MEAS3;
        }
    }

    //vlastni mereni - pravdepodoben vyuzitim i_proc_stage retezce
    void __proc_measurement() {

        cv::Mat src(info.h, info.w, CV_8U, img);
        th.proc(0, &src);

        QVector<QRgb> colorTable;
        for (int i = 0; i < 256; i++)
            colorTable.push_back(QColor(i, i, i).rgb());

        QImage meas(r_fl.loc.ptr(), r_fl.loc.cols, r_fl.loc.rows, QImage::Format_Indexed8);
        meas.setColorTable(colorTable);

        //vysetreni primky vuci tolerancnimu poli
        int D = par["toler-zone"].get().toInt() / 2;
        int G = par["toler-gap"].get().toInt() / 2 ;
        int X = par["toler-offset"].get().toInt() / 2;

        meas = meas.convertToFormat(QImage::Format_RGB32);
        QPainter toler;
        toler.begin(&meas);
        toler.setPen(QColor(255, 0, 0));
        toler.drawLine(X - D/2, 0, X - D/2, info.h/2-1);
        toler.setPen(QColor(0, 255, 0));
        toler.drawLine(X + D/2, 0, X + D/2, info.h/2-1);
        toler.end();

        store.insert(meas);
        emit present_meas(meas, 0, 0);    //vizualizace preview kamery
    }

    //priprava datagramu pred odeslanim - uzitim private hodnot 
    void __serialize_measurement_res(unsigned ord, QByteArray &to) {

        uint32_t i_perc_lfit = 10 * perc_lfit;
        uint32_t i_perc_rfit = 10 * perc_rfit;
        uint32_t i_mm_gap = 10 * mm_gap;

        //typedef struct {
        //  uint8_t sync[3];
        //  uint8_t ord;
        //  uint32_t flags;
        //  uint32_t leftp;
        //  uint32_t rightp;
        //  uint32_t gap;
        //  uint32_t count;
        //} stru_inteva_dgram;
        stru_inteva_dgram d = {
            sync_inveva_pattern[0], sync_inveva_pattern[1], sync_inveva_pattern[2],
            ord,
            error_mask,
            i_perc_lfit, i_perc_rfit, i_mm_gap,
            meas_count
        };

        to = QByteArray((const char *)&d, sizeof(d));
    }

    //rozklad datagramu do private hodnot 
    void __deserialize_measurement_res(unsigned ord, QByteArray &from) {

        //todo: nic nepotrebujem asi jen vyhodnoceni chyby ridici strany
        ord = ord;
        from = from;
    }	

public:
    t_inteva_app(t_comm_tcp_inteva &comm,
                 QString &js_config,
                 QString &pt_storage) :
        i_collection(js_config, pt_storage, &comm),
        th(path),
        l_fl(),  //default konfigurace
        r_fl()
    {
        //zretezeni analyz - detekce linek oboji je napojena na 
        QObject::connect(&th, SIGNAL(next(int, void *)), &l_fl, SLOT(proc(int, void *)));
        QObject::connect(&th, SIGNAL(next(int, void *)), &r_fl, SLOT(proc(int, void *)));

        //specialni nastaveni pro aproximace primek
        QString pname = "search-from";
        QVariant pval;

        pval = 0; l_fl.config(pname, &pval);
        pval = 2; r_fl.config(pname, &pval);

        pname = "fitline-offs-left";
        pval = 300; l_fl.config(pname, &pval);
        pname = "fitline-offs-right";
        pval = 300; r_fl.config(pname, &pval);

        //todo - set individual roi if necessery
    }

    virtual ~t_inteva_app(){

    }
};

#endif //T_INTEVA_TESNENI
