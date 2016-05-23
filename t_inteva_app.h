#ifndef T_INTEVA_TESNENI
#define T_INTEVA_TESNENI

#include <QObject>
#include <stdio.h>
#include "i_collection.h"

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

    //preproces - spusten pred snimanim obrazu
    void __init_measurement() {

        perc_lfit = perc_rfit = 0;
        mm_gap = 0.0;
    }

    //potprocess - zpracovani hodnot z analyz 
    void __eval_measurement() {

        //TODO: vysetreni primky vuci tolerancnimu poli
        //prepocet z pix na mm
    }

    //vlastni mereni - pravdepodoben vyuzitim i_proc_stage retezce
    void __proc_measurement() {

        cv::Mat src(info.h, info.w, CV_8U, img);
        th.proc(0, &src);
    }

    //priprava datagramu pred odeslanim - uzitim private hodnot 
    unsigned __serialize_measurement_res(void *to, unsigned reserved) {

        if(reserved < 3*sizeof(uint32_t))
            return 0;

        uint8_t *pto = (uint8_t *)to;
        uint32_t i_perc_lfit = 10 * perc_lfit;
        uint32_t i_perc_rfit = 10 * perc_rfit;
        uint32_t i_mm_gap = 10 * mm_gap;

        memcpy(pto, &i_perc_lfit, sizeof(uint32_t)); pto += sizeof(uint32_t);
        memcpy(pto, &i_perc_rfit, sizeof(uint32_t)); pto += sizeof(uint32_t);
        memcpy(pto, &i_mm_gap, sizeof(uint32_t)); pto += sizeof(uint32_t);

        return pto - (uint8_t *)to;
    }

    //rozklad datagramu do private hodnot 
    void __deserialize_measurement_res(void *from, unsigned reserved) {

        if(reserved < 3*sizeof(uint32_t))
            return;

        uint8_t *pfrom = (uint8_t *)from;
        uint32_t i_perc_lfit;
        uint32_t i_perc_rfit;
        uint32_t i_mm_gap;

        memcpy(&i_perc_lfit, pfrom, sizeof(uint32_t)); pfrom += sizeof(uint32_t);
        memcpy(&i_perc_rfit, pfrom, sizeof(uint32_t)); pfrom += sizeof(uint32_t);
        memcpy(&i_mm_gap, pfrom, sizeof(uint32_t)); pfrom += sizeof(uint32_t);

        perc_lfit = i_perc_lfit / 10.0;
        perc_rfit = i_perc_rfit / 10.0;
        mm_gap = i_mm_gap / 10.0;
    }	

public:
    t_inteva_app(t_comm_tcp_inteva &comm, QString &js_config) :
        i_collection(js_config, &comm, this),
        th(path),
        l_fl(),
        r_fl()
    {
        //zretezeni analyz - detekce linek oboji je napojena na 
        QObject::connect(&th, SIGNAL(next(int, void *)), &l_fl, SLOT(proc(int, void *)));
        QObject::connect(&th, SIGNAL(next(int, void *)), &r_fl, SLOT(proc(int, void *)));

        QVariant left_v = 0;
        l_fl.config(QString("search_from"), &left_v);

        QVariant rigth_v = 0;
        r_fl.config(QString("search_from"), &rigth_v);

        //todo - set individual roi if necessery
    }

    virtual ~t_inteva_app(){

    }
};

#endif //T_INTEVA_TESNENI
