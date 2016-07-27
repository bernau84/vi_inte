#ifndef T_INTEVA_TESNENI
#define T_INTEVA_TESNENI

#include <QObject>
#include <QPainter>

#include <stdio.h>
#include <algorithm>
#include "i_collection.h"
#include "t_inteva_specification.h"

#include "processing/t_vi_proc_fitline.h"
#include "processing/t_vi_proc_statistic.h"
#include "processing/t_vi_proc_threshold_cont.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

//to do - vyresit jak uchovat a kruhove ukladat 
//stdout v bashi

class t_inteva_app : public i_collection {
	
	Q_OBJECT

private:

    t_vi_proc_statistic st;
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

        qDebug() << "t_inteva_app::__init_measurement";

        perc_lfit = perc_rfit = 0;
        mm_gap = 0.0;
        meas_count = 0;
    }

    //potprocess - zpracovani hodnot z analyz 
    void __eval_measurement() {

        meas_count++;
        qDebug() << "t_inteva_app::__eval_measurement" << meas_count;

        //vysetreni primky vuci tolerancnimu poli
        int D = par["toler-zone"].get().toInt();
        int G = par["toler-gap"].get().toInt();
        int X = par["toler-offset"].get().toInt();

        log += QString("meas: settings - zone = %1, x-shift = %2, gap = %3\r\n").arg(D).arg(X).arg(G);

        int me_x = (r_fl.line[2] + l_fl.line[2]) / 2;
        int me_d = r_fl.line[2] - l_fl.line[2];  //stredni vzdalenost primek
        int up_d_r = x_crossing(r_fl.line, 0);
        int up_d_l = x_crossing(l_fl.line, 0);  //horni vzdalenost
        int dw_d_r = x_crossing(r_fl.line, info.h);
        int dw_d_l = x_crossing(l_fl.line, info.h);  //spodni

        log += QString("meas: left line profile top %1, bottom %2\r\n").arg(up_d_l).arg(dw_d_l);
        log += QString("meas: right line profile top %1, bottom %2\r\n").arg(up_d_r).arg(dw_d_r);

        int perc_lfit_up = up_d_l - (X - D/2);
        int perc_lfit_dw = dw_d_l - (X - D/2);

        int perc_rfit_up = (X + D/2) - up_d_r;
        int perc_rfit_dw = (X + D/2) - dw_d_r;

        //pokud jsou oba kraje uvnitr tolerance tak 100% jinak primka vycuhuje v pomeru min / max
        perc_lfit = 0;
        if((perc_lfit_up * perc_lfit_dw) < 0){

            double hi = fabs(double(max(perc_lfit_up, perc_lfit_dw)));
            double lo = fabs(double(min(perc_lfit_up, perc_lfit_dw)));
            perc_lfit = 100 * (hi / (hi + lo));
        } else if((perc_lfit_up < D) && (perc_lfit_dw < D) &&
                  (perc_lfit_up > 0) && (perc_lfit_dw > 0)){

            perc_lfit = 100;
        }

        perc_rfit = 0;
        if((perc_rfit_up * perc_rfit_dw) < 0){

            double hi = fabs(double(max(perc_rfit_up, perc_rfit_dw)));
            double lo = fabs(double(min(perc_rfit_up, perc_rfit_dw)));
            perc_rfit = 100 * (hi / (hi + lo));
        } else if((perc_rfit_up < D) && (perc_rfit_dw < D) &&
                  (perc_rfit_up > 0) && (perc_rfit_dw > 0)){

            perc_rfit = 100;
        }

        mm_gap = (me_d + (up_d_r - up_d_l) + (dw_d_r - dw_d_l)) / 3;

        if(mm_gap > G){

            error_mask &= ~VI_ERR_OK;
            error_mask |= VI_ERR_MEAS3;
            log += QString("meas: VI_ERR_MEAS3(%1) - %2 pix gap over %3 pix limit\r\n").arg(VI_ERR_MEAS3, 0, 16).arg(mm_gap).arg(G);
        }

        if(perc_rfit < 100){

            error_mask &= ~VI_ERR_OK;
            error_mask |= VI_ERR_MEAS1;
            log += QString("meas: VI_ERR_MEAS1(%1) - only %2%% fit inside right limit\r\n").arg(VI_ERR_MEAS1, 0, 16).arg(perc_rfit);
        }

        if(perc_lfit < 100){

            error_mask &= ~VI_ERR_OK;
            error_mask |= VI_ERR_MEAS2;
            log += QString("meas: VI_ERR_MEAS2(%1) - only %2%% fit inside left limit\r\n").arg(VI_ERR_MEAS2, 0, 16).arg(perc_lfit);
        }

        log += QString("meas: results - left %1%%, right %2%%, gap %3 pix\r\n").arg(perc_lfit).arg(perc_rfit).arg(mm_gap);

        QVector<QRgb> colorTable;
        for (int i = 0; i < 256; i++)
            colorTable.push_back(QColor(i, i, i).rgb());

        QImage meas(r_fl.loc.ptr(), r_fl.loc.cols, r_fl.loc.rows, QImage::Format_Indexed8);
        meas.setColorTable(colorTable);

        meas = meas.convertToFormat(QImage::Format_ARGB32);
        QPainter toler;
        QPen pen_red(QColor(255, 0, 0));
        pen_red.setWidth(2);
        QPen pen_blue(QColor(0, 0, 255));
        pen_blue.setWidth(2);

        toler.begin(&meas);
        toler.setPen(pen_red);
        toler.setBrush(QBrush(QColor(255, 0, 0, 32)));
        toler.drawRect(QRect((X - D/2)/2, 5, D/2, info.h/2-5));  //lomeno 2 kvuli zmenseni r_fl.loc vuci puvodnimu meritku
        toler.setPen(pen_blue);
        toler.setBrush(QBrush(QColor(0, 0, 255, 32)));
        toler.drawRect(QRect((me_x - G/2)/2, 5, G/2, info.h/2-5));
        toler.end();

        QEventLoop loop;  //process drawings
        loop.processEvents();
        loop.processEvents();
        loop.processEvents();

        store.insert(meas);
        emit present_meas(meas, perc_lfit, perc_rfit);    //vizualizace preview kamery

        //prepocet z pix na mm - provadime jem pokud mame zakladni kalibraci
        if(!par.ask("plane-cam-distance-mm") || !par.ask("focal-length-mm"))
            return;

        int f = par["focal-length-mm"].get().toInt();
        int d = par["plane-cam-distance-mm"].get().toInt();

        mm_gap *= d;
        mm_gap /= f; //to je stale v pix
        mm_gap *= float(2.2e-3); //rozliseni kamery - v mm/pix; vysledek v mm

        log += QString("meas: gap %1 mm (f = %2, d = %3)\r\n").arg(mm_gap).arg(f).arg(d);
    }

    //vlastni mereni - pravdepodoben vyuzitim i_proc_stage retezce
    void __proc_measurement() {

        qDebug() << "t_inteva_app::__proc_measurement";

        cv::Mat src(info.h, info.w, CV_8U, img);
        st.proc(0, &src);

        float hist = 0.0;
        int auto_th, max_th = par["threshold_positive"].get().toInt();
        float auto_th_perc = par["threshold_hist_adapt_perc"].get().toDouble();

        st.proc(t_vi_proc_statistic::t_vi_proc_statistic_ord::STATISTIC_HIST_BRIGHTNESS, &src);
        qDebug() << "t_inteva_app::__proc_measurement histogram";

        for(auto_th = 0; auto_th < max_th; auto_th++){

            hist += st.out.at<float>(auto_th);
            qDebug() << "t_inteva_app::__proc_measurement hist-cum-sum" << hist;
            if((hist / (src.rows * src.cols)) > (auto_th_perc/100.0))
                break;
        }

        QString pname;
        QVariant pval;

        pname = "threshold_positive";
        pval = auto_th; th.config(pname, &pval);
        qDebug() << "t_inteva_app::__proc_measurement threshold update to " << auto_th;

        th.proc(0, &src);
        qDebug() << "t_inteva_app::__proc_measurement threshold+line-fit";
    }

    //priprava datagramu pred odeslanim - uzitim private hodnot 
    void __serialize_measurement_res(unsigned ord, QByteArray &to) {

        qDebug() << "t_inteva_app::__serialize_measurement_res";

        uint32_t i_perc_lfit = 10 * perc_lfit;
        uint32_t i_perc_rfit = 10 * perc_rfit;
        uint32_t i_um_gap = 1000 * mm_gap;

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
            i_perc_lfit, i_perc_rfit, i_um_gap,
            meas_count
        };

        to = QByteArray((const char *)&d, sizeof(d));
    }

    //rozklad datagramu do private hodnot 
    void __deserialize_measurement_res(unsigned ord, QByteArray &from) {

        qDebug() << "t_inteva_app::__deserialize_measurement_res";

        ord = ord;
        from = from;
    }	

public:
    t_inteva_app(t_comm_tcp_inteva &comm,
                 QString &js_config,
                 QString &pt_storage) :
        i_collection(js_config, pt_storage, &comm),
        st(),
        th(path),
        l_fl(),  //default konfigurace
        r_fl()
    {
        qDebug() << "t_inteva_app::t_inteva_app()";

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
        pval = 300; r_fl.config(pname, &pval);
        pname = "fitline-offs-right";
        pval = 300; l_fl.config(pname, &pval);
        pval = 300; r_fl.config(pname, &pval);

        perc_lfit = perc_rfit = mm_gap = -1.0;

        //todo - set individual roi if necessery
    }

    virtual ~t_inteva_app(){

    }
};

#endif //T_INTEVA_TESNENI
