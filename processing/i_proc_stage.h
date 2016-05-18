#ifndef I_PROC_STAGE_H
#define I_PROC_STAGE_H

#include <QObject>
#include <QFile>
#include <QJsonDocument>
#include <QElapsedTimer>

#include "../t_vi_setup.h"

static int stage_counter = 0;

class i_proc_stage : public QObject
{
    Q_OBJECT

private:
    //data processing
    virtual int iproc(int p1, void *p2) = 0;

    //constructor helpers
    QJsonObject __set_from_file(const QString &path){

        // default config
        QFile f_def(path);  //from resources
        if(f_def.open(QIODevice::ReadOnly | QIODevice::Text)){

            QByteArray f_data = f_def.read(64000);

            QJsonDocument js_doc = QJsonDocument::fromJson(f_data);
            if(!js_doc.isEmpty()){

                //qDebug() << js_doc.toJson();
                return js_doc.object();
            }
        }

        return QJsonObject();
    }

protected:
   t_collection par;
   QElapsedTimer etimer;
   qint64 elapsed;
   QString fancy_name;

public slots:
   //data processor
   int proc(int p1, void *p2){

        //FIXME: musi byt v ramci iproc + emit by se melo naopak delat v ramci slotu!
            //iproc musi vracet p1, p2 pro emitovani signalu (pripadne chybu ze se emitovat nebude)

       etimer.start();
       int ret = iproc(p1, p2);
       if(elapsed == 0)
           elapsed = etimer.elapsed();

       qDebug() << QString("processssing %1 take %2ms").arg(fancy_name).arg(elapsed);
       return ret;
   }

   //init privates from configuration
   virtual int reload(int p) = 0;

signals:
    void next(int p1, void *p2);

public:
    //read / update config parameter runtime
    //new value is not promoted to json file
    QVariant config(QString &name, QVariant *value){

        if(!par.ask(name))
            return QVariant();

        if(value){

            QJsonValue jval_set = QJsonValue::fromVariant(*value);
            QJsonValue jval_get = par[name].set(jval_set);

            reload(0);

            return jval_get.toVariant();
        }

        return par[name].get().toVariant();
    }

    double get_last_procesed_mstime(){

        return elapsed / 1000.0;
    }

    i_proc_stage(const QString &js_config, QObject *parent = NULL):
        QObject(parent),
        par(__set_from_file(js_config))
    {
        fancy_name = QString("stage%1").arg(stage_counter++);
        elapsed = 0;
    }

    virtual ~i_proc_stage(){;}
};

#endif // I_PROC_STAGE_H
