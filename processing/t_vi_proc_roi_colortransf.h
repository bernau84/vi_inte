#ifndef T_VI_PROC_COLORTRANSF_H
#define T_VI_PROC_COLORTRANSF_H

#include "i_proc_stage.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

static const QString proc_colortransf_defconfigpath(":/js_config_roi_colortransf.txt");

class t_vi_proc_colortransf : public i_proc_stage
{
private:
    Mat out;
    cv::Rect roi;
    int cvt;

public:
    t_vi_proc_colortransf(const QString &path = proc_colortransf_defconfigpath):
        i_proc_stage(path)
    {
        fancy_name = "colortransf (" + fancy_name + ")";
        reload(0);
        qDebug() << "ColorTr & ROI import setup:" << cvt << roi.x << roi.y << roi.width << roi.height;
    }


    virtual ~t_vi_proc_colortransf(){;}

public slots:

    int reload(int p){

        p = p;

        roi.x = roi.y = roi.width = roi.height = 0;

        roi.x = par["roi-center-x"].get().toInt();   //from collection
        roi.y = par["roi-center-y"].get().toInt();
        roi.width = par["roi-width"].get().toInt();
        roi.height = par["roi-height"].get().toInt();

        if(0 > (cvt = par["color-transf"].get().toInt()))
            cvt = CV_BGR2GRAY;

        return 1;
    }

private:
    int iproc(int p1, void *p2){

        int t_cvt = (p1 <= 0) ? cvt : p1;
        Mat *src = (Mat *)p2;

        ///Convert image to gray
        cv::cvtColor(*src, out, t_cvt /*CV_BGR2GRAY*/);

        ///ROI
        if(roi.width && roi.height){

            Mat cropped = out(roi);
            out = cropped.clone();   //deep copy
        }

        elapsed = etimer.elapsed();
        emit next(1, &out);
        return 1;
    }

};

#endif // T_VI_PROC_COLORTRANSF_H
