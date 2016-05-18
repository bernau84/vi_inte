#ifndef T_CAMERA_ATTRIB
#define T_CAMERA_ATTRIB

#include "../t_vi_setup.h"
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

class t_vi_camera_attrib : public t_collection {

private:

    //constructor helpers
    QJsonObject __set_from_file(const QString &path){

        // default config
        QFile f_def(path);  //from resources
        if(f_def.open(QIODevice::ReadOnly | QIODevice::Text)){

            QByteArray f_data = f_def.read(64000);
            QJsonDocument js_doc = QJsonDocument::fromJson(f_data);
            if(!js_doc.isEmpty())
                return js_doc.object();
        }

        return QJsonObject();
    }


public:
    t_vi_camera_attrib(const QString &path = "js_camera_base.txt"):
        t_collection(__set_from_file(path))
    {

    }

    ~t_vi_camera_attrib(){;}
};

#endif // T_CAMERA_ATTRIB

