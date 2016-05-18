#ifndef T_VI_CAMERA_WEB_USB_H
#define T_VI_CAMERA_WEB_USB_H

#include "../i_camera_base.h"

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/legacy/compat.hpp>

using namespace std;
using namespace cv;

class t_vi_camera_web_usb : public i_vi_camera_base
{
private:
    cv::VideoCapture capture;

public:

    /*! \brief before fist grab / for reset do original state */
    int init(){

        if (!capture.isOpened()) {
            // if camera not found, display an error message
            sta = CAMSTA_ERROR;
            return -1;
        }

        capture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
        capture.set(CV_CAP_PROP_FRAME_HEIGHT, 720);

        sta = CAMSTA_PREPARED;
        return 0;
    }

    int snap(void *img, unsigned free, i_vi_camera_base::t_campic_info *info = NULL){

        if(sta != CAMSTA_PREPARED)
            return -1;

        cv::Mat ocv_grabed;
        if(!capture.read(ocv_grabed)){

            return -101;
        }

        qDebug() << "Grabbed channels: " << QString::number(ocv_grabed.channels());

        cv::Mat ocv_rgb;
        switch(ocv_grabed.channels()){
            case 1:
                cvtColor(ocv_grabed, ocv_rgb, CV_GRAY2RGBA);
            break;
            case 3:
                cvtColor(ocv_grabed, ocv_rgb, CV_BGR2RGBA);
            break;
            case 4:
               cvtColor(ocv_grabed, ocv_rgb, CV_BGRA2RGBA);
            break;
            default:
                qDebug() << "Unsupported format of grabbed picture!";
                return -102;
            break;
        }

        QImage picFile(ocv_rgb.ptr(), ocv_rgb.cols, ocv_rgb.rows, ocv_rgb.step, QImage::Format_RGB32);
        if(picFile.isNull())
            return 0;

        int ret = picFile.byteCount();
        if(ret > (int)free)
            return -1;

        qDebug() << "file-pic-size:" << ret <<
                    "file-pic-x:" << picFile.width() <<
                    "file-pic-y:" << picFile.height();

        /*! convertToFormat() according to setup */
        return convertf(picFile, img, free, info);
    }

    t_vi_camera_web_usb():
        capture(0)  // open a first USB camera found (0-based camera index)
    {
    }

    t_vi_camera_web_usb(const QString &path):
            i_vi_camera_base(path),
            capture(0)  // open a first USB camera found (0-based camera index)
    {
    }

    ~t_vi_camera_web_usb(){
    }
};

#endif // T_VI_CAMERA_WEB_USB_H
