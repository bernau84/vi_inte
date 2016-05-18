#ifndef T_VI_PROC_RECTIFY
#define T_VI_PROC_RECTIFY

#include "i_proc_stage.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

/*! \todo - nechavame az budu znat kalibracni parametry
ve formatu ocv
*/

using namespace cv;
using namespace std;

static const QString proc_rectify_defconfigpath(":/js_config_rectify.txt");

class t_vi_proc_rectify : public i_proc_stage
{
private:
    QList<QVariant> cam_param;
    QList<QVariant> dist_param;

public:

    Mat out;

    t_vi_proc_rectify(const QString &path = proc_rectify_defconfigpath):
        i_proc_stage(path)
    {
        fancy_name = "rectification(" + fancy_name + ")";
        reload(0);
        qDebug() << "Rectification camera-matrix params no" << cam_param.size();
        qDebug() << "Rectification distorsion params no" << dist_param.size();
    }


    virtual ~t_vi_proc_rectify(){;}

public slots:
    int reload(int p){

        p = p;
        cam_param = par["instrict"].get().toArray().toVariantList();   //from collection
        dist_param = par["distorsion"].get().toArray().toVariantList();
        return 1;
    }

private:
    int iproc(int p1, void *p2){

        p1 = p1;
        Mat *src = (Mat *)p2;

        qDebug() << QString("undistortion pic channel no %1").arg(src->channels());

        if((cam_param.size() != 9) || (dist_param.size() != 4)){

            emit next(p1, src);  //zjednodusime si to a poslem p1 dale
            return 0;
        }

        Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
        Mat distCoeffs = Mat::zeros(8, 1, CV_64F);

        cameraMatrix.at<double>(0,0) = cam_param[0].toDouble(); //fx
        cameraMatrix.at<double>(1,1) = cam_param[4].toDouble(); //fy
        cameraMatrix.at<double>(0,2) = cam_param[2].toDouble(); //cx
        cameraMatrix.at<double>(1,2) = cam_param[5].toDouble(); //cy

        distCoeffs.at<double>(0) = dist_param[0].toDouble();
        distCoeffs.at<double>(1) = dist_param[1].toDouble();
        distCoeffs.at<double>(2) = dist_param[2].toDouble();
        distCoeffs.at<double>(3) = dist_param[3].toDouble();

        cv::undistort(*src, out, cameraMatrix, distCoeffs);
        //imwrite("undistoreted.bmp", out);

        elapsed = etimer.elapsed();
        emit next(p1, &out); //zjednodusime si to a poslem p1 dale
        return 1;
    }

};

#endif // T_VI_PROC_RECTIFY

