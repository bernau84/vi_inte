#include <QCoreApplication>

#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

#include "t_vi_proc_roi_colortransf.h"
#include "t_vi_proc_threshold_cont.h"
#include "t_vi_proc_statistic.h"
#include "t_vi_proc_fitline.h"


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    /// Load source image and convert it to gray
    //Mat src = imread( "c:\\Users\\bernau84\\turnov-meroll-daylight.bmp" );
    //Mat src = imread( "c:\\Users\\bernau84\\Documents\\sandbox\\build-roll_idn_coll-Desktop_Qt_5_4_2_MSVC2010_OpenGL_32bit-Debug\\pic29.bmp", 1 );
    //Mat src = imread( "c:\\Users\\bernau84\\Documents\\sandbox\\roll_idn\\build-processing-Desktop_Qt_5_4_1_MSVC2010_OpenGL_32bit-Debug\\debug\\smp.bmp" );
    //Mat src = imread( "c:\\Users\\bernau84\\Pictures\\test_roll_idn3.bmp");
    //Mat src = imread( "c:\\mares\\Workspace\\vi_rollidn\\processing\\test_roll_idn3.bmp");
    //Mat src = imread("c:\\Users\\bernau84\\Pictures\\trima_daybackground\\trn_meas_exp1_1_m1.bmp");
    //Mat src = imread("c:\\Users\\bernau84\\Pictures\\trima_precision_samples\\turnov-meroll-daylight-left-ex3-705x265mm_sel.bmp");
    //Mat src = imread("c:\\Users\\bernau84\\Pictures\\trima_precision_samples\\roll_1300mm_floor_prec2.bmp");
    //xMat src = imread("c:\\Users\\bernau84\\Pictures\\trima_precision_samples\\turnov_fixedpos_a2_705x265_sel.bmp");
    //Mat src = imread("c:\\Users\\bernau84\\Pictures\\trima_precision_samples\\turnov-meroll-daylight-left-ex3-705x265mm_sel.bmp");
    //Mat src = imread("c:\\Users\\bernau84\\Pictures\\trima_precision_samples\\turnov_hi_150mm_1_selection_sel.bmp");
    //xMat src = imread("c:\\Users\\bernau84\\Pictures\\trima_precision_samples\\turnov-big-2-sel.bmp");
    //xMat src = imread("c:\\Users\\bernau84\\Pictures\\trima_precision_samples\\turnov_hi_70mm_lodia_5_Ldistance_sel.bmp");
    //Mat src = imread("4_in_stinitko.bmp", CV_LOAD_IMAGE_GRAYSCALE);
    Mat src = imread("c:\\Users\\bernau\\Projects\\sandbox\\pic5_50.bmp", CV_LOAD_IMAGE_GRAYSCALE);

    t_vi_proc_statistic st("config.txt");
    t_vi_proc_threshold th("config.txt");
    t_vi_proc_fitline l_fl("config.txt");
    t_vi_proc_fitline r_fl("config.txt");

    float hist = 0.0;
    int dthreshold = 30;
    st.proc(t_vi_proc_statistic::t_vi_proc_statistic_ord::STATISTIC_HIST_BRIGHTNESS, &src);
    for(dthreshold = 0; dthreshold <256; dthreshold ++){

        hist = st.out.at<float>(dthreshold);
        qDebug() << dthreshold << hist;
        if(hist / (src.rows * src.cols) > 0.002)  //prah 1 promile
            break;
    }

    QString pname;
    QVariant pval;

    pname = "threshold_positive";
    pval = dthreshold; th.config(pname, &pval);

    pname = "search-from";
    pval = 0; l_fl.config(pname, &pval);
    pval = 2; r_fl.config(pname, &pval);

    pname = "fitline-offs-left";
    pval = 300; l_fl.config(pname, &pval);
    pval = 300; r_fl.config(pname, &pval);

    pname = "fitline-offs-right";
    pval = 300; l_fl.config(pname, &pval);
    pval = 300; r_fl.config(pname, &pval);

//    QObject::connect(&th, SIGNAL(next(int, void *)), &l_fl, SLOT(proc(int, void *)));
//    QObject::connect(&th, SIGNAL(next(int, void *)), &r_fl, SLOT(proc(int, void *)));

    th.proc(0, &src);

    return a.exec();
}
