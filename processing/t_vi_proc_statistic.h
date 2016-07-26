#ifndef T_VI_PROC_STATISTIC_H
#define T_VI_PROC_STATISTIC_H


#include "i_proc_stage.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

//#define VI_PROC_STA_OUTPUT  1

static const QString proc_statistic_defconfigpath(":/js_config_statistic.txt");

//vypocet pozadovaneho parametru nad obrazkem
class t_vi_proc_statistic : public i_proc_stage
{
private:

public:

    enum t_vi_proc_statistic_ord {
        STATISTIC_NONE = 0,
        STATISTIC_MEAN,     //mean vale over all pixels and channels
        STATISTIC_BRIGHTNESS,  //color channel weighted mean
        STATISTIC_HIST_X,
        STATISTIC_HIST_Y,
        STATISTIC_HIST_BRIGHTNESS,
    };

    cv::Mat out;

    t_vi_proc_statistic(const QString &path = proc_statistic_defconfigpath):
        i_proc_stage(path)
    {
        fancy_name = "image-statistics(" + fancy_name + ")";
        reload(0);
    }


    virtual ~t_vi_proc_statistic(){;}

public slots:

    int reload(int p){

        p = p;
        return 1;
    }

private:
    int iproc(int p1, void *p2){

        t_vi_proc_statistic_ord ord = (t_vi_proc_statistic_ord)p1;
        Mat *src = (Mat *)p2;

        switch(ord){

            case STATISTIC_MEAN:
            {
                float res = 0.0;
                out = Mat(1, 1, CV_32FC1);
                cv::Scalar tres = cv::mean(*src);
                for(int i=0; i<tres.channels; i++)
                    res += tres[i];

                out.at<float>(0) = float(res/tres.channels);
            }
            break;
            /*! assumes RGB or Mono picture format */
            case STATISTIC_BRIGHTNESS:
            {
                float res = 0.0;
                out = Mat(1, 1, CV_32FC1);
                cv::Scalar tres = cv::mean(*src);

                double weights[3] = {0.299, 0.587, 0.144};  //0.299*R + 0.587*G + 0.144*B

                switch(tres.channels){
                    case 1:
                        res = tres[0];
                    break;
                    case 3:
                    case 4:
                        for(int i=0; i<3; i++)
                            res += tres[i] * weights[i];
                    break;
                }

                out.at<float>(0) = res;
            }
            break;
            case STATISTIC_HIST_X:

                cv::reduce(*src, out, 0, CV_REDUCE_AVG);

                for(int x=0; x<(src->cols)-1; x++)
                    cv::line(*src, Point(x, out.at<uchar>(x)),
                                    Point(x+1, out.at<uchar>(x+1)),
                                    Scalar(255, 0, 0), 3, 8);

#ifdef VI_PROC_STA_OUTPUT
                cv::imwrite("sta-hist-x.bmp", *src);
                qDebug() << "t_vi_proc_statistic::iproc imwrite sta-hist-x.bmp";
#endif //VI_PROC_STA_OUTPUT
            break;
            case STATISTIC_HIST_Y:

                cv::reduce(*src, out, 1, CV_REDUCE_AVG);

                for(int y=0; y<(src->rows)-1; y++)
                    cv::line(*src, Point(out.at<uchar>(y), y),
                             Point(out.at<uchar>(y+1), y+1),
                             Scalar(255, 0, 0), 3, 8);

#ifdef VI_PROC_STA_OUTPUT
                cv::imwrite("sta-hist-y.bmp", *src);
                qDebug() << "t_vi_proc_threshold::iproc imwrite sta-hist-y.bmp";
#endif //VI_PROC_STA_OUTPUT

            break;
            case STATISTIC_HIST_BRIGHTNESS:
            {
                unsigned chsize = src->channels();
                float range[2] = {0, 256};
                int channels[32/*CV_MAX_DIMS*/];
                int histSize[32/*CV_MAX_DIMS*/];
                const float *ranges[32/*CV_MAX_DIMS*/];

                for(int ch=0; ch<chsize; ch++){

                    channels[ch] = ch;
                    histSize[ch] = 256;
                    ranges[ch] = range;
                }

                cv::calcHist(src, 1, channels, Mat(),
                             out, chsize,
                             histSize, ranges);
#ifdef VI_PROC_STA_OUTPUT
                for(int c=0; c<255; c++){

                    if(chsize > 0)
                        cv::line(*src, Point(c, out.at<float>(c, 0)/200),
                                 Point(c+1, out.at<float>(c+1, 0)/200),
                                 Scalar(128, 0, 0), 3, 8);
                    if(chsize > 1)
                        cv::line(*src, Point(c, out.at<float>(c, 1)/200),
                                 Point(c+1, out.at<float>(c+1, 1)/200),
                                 Scalar(0, 128, 0), 3, 8);
                    if(chsize > 2)
                        cv::line(*src, Point(c, out.at<float>(c, 2)/200),
                                 Point(c+1, out.at<float>(c+1, 2/200)),
                                 Scalar(0, 0, 128), 3, 8);
                }

                /// Show in a window
                cv::imwrite("sta-color-hist.bmp", *src);
                qDebug() << "t_vi_proc_threshold::iproc imwrite sta-color-hist.bmp";
#endif //VI_PROC_STA_OUTPUT
            }
            break;
            default:
            break;
        }

        elapsed = etimer.elapsed();
        emit next(1, &out);
        return 1;
    }

};
#endif // T_VI_PROC_STATISTIC_H

