#ifndef T_VI_CAMERA_OFFLINE_FILE_H
#define T_VI_CAMERA_OFFLINE_FILE_H

#include "../i_camera_base.h"

#include <QtWidgets/QFileDialog>

class t_vi_camera_offline_file : public i_vi_camera_base
{
public:

    int isnap(void *img, unsigned free, i_vi_camera_base::t_campic_info *info = NULL){

        QString picName = QFileDialog::getOpenFileName(NULL, "Open Image", "", "Image Files (*.png *.jpg *.bmp)");
        if(picName.isNull())
            return 0;

        QImage picFile(picName);
        if(picFile.isNull())
            return 0;

        QImage picRGBA = picFile.convertToFormat(QImage::Format_RGB32); //vzdy protoze z nej umim udelat grayscale, nebo to zustane tak jak je

        qDebug() << "file-pic-size:" << picRGBA.byteCount() <<
                    "file-pic-x:" << picRGBA.width() <<
                    "file-pic-y:" << picRGBA.height();

        t_campic src, out;

        out.p = img;
        out.size = free;

        src.p = picRGBA.bits();
        src.size = picRGBA.byteCount();
        src.t.format = CAMF_32bRGB;
        src.t.w = picRGBA.width();
        src.t.h = picRGBA.height();


        /*! convertToFormat() according to setup */
        int ret = convertf(src, out);
        if(info) *info = out.t;
        return ret;
    }

    t_vi_camera_offline_file()
    {
    }

    t_vi_camera_offline_file(const QString &path):
            i_vi_camera_base(path)
    {
    }

    virtual ~t_vi_camera_offline_file(){
    }
};

#endif // T_VI_CAMERA_OFFLINE_FILE_H
