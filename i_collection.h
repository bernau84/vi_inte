#ifndef I_COLLECTION
#define I_COLLECTION

#include <QObject>
#include <QEventLoop>
#include <QElapsedTimer>

#include <stdio.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "cinterface/i_comm_generic.h"

#include "cameras/basler/t_vi_camera_basler_usb.h"
#include "cameras/basler/t_vi_camera_basler_gige.h"
#include "cameras/offline/t_vi_camera_offline_file.h"

#include "t_vi_setup.h"
#include "t_inteva_specification.h"
#include "t_record_storage.h"

//global definition of txt remote orders
extern const char *ords[];

class i_collection : public QObject {

    Q_OBJECT

protected:

    QElapsedTimer etimer;
    QElapsedTimer ptimer;

    QImage snapshot;
    QString path;
    QString log;

    uint8_t *img;
    i_vi_camera_base::t_campic_info info;

    t_vi_setup par;
    bool abort;
    uint32_t error_mask;  //suma flagu e_vi_plc_pc_errors

#ifdef USE_USB
    t_vi_camera_basler_usb cam_device;
#elif defined USE_GIGE
    t_vi_camera_basler_gige cam_device;
#endif //USE_XXX

    t_vi_camera_offline_file cam_simul;  //pokud neni pripojena zadna basler kamera 
        //pracuje diky tomu s offline fotkami

    i_comm_generic *iface;  //komunikacni iterface

    t_record_storage store;  //ukladani vysledku a logu do kruhoveho bufferu 

    //constructor helpers
    QJsonObject __from_file(){

        qDebug() << "i_collection::__from_file(): enter";

        // default config
        QFile f_def(path);  //from resources
        if(f_def.open(QIODevice::ReadOnly | QIODevice::Text)){

            QByteArray f_data = f_def.read(64000);
            qDebug() << "i_collection::__from_file(): bytes " << f_data.left(32).simplified() << "...";

            QJsonDocument js_doc = QJsonDocument::fromJson(f_data);
            if(!js_doc.isEmpty()){

                qDebug() << "i_collection::__from_file(): json " << js_doc.toJson().left(32).simplified() << "...";
                return js_doc.object();
            }
        }

        qDebug() << "i_collection::__from_file(): file not found!";
        return QJsonObject();
    }

    //constructor helpers
    bool __to_file(){

        qDebug() << "i_collection::__to_file(): enter";

        // default config
        QFile f_def(path);  //from resources
        if(f_def.open(QIODevice::WriteOnly | QIODevice::Text)){

            QJsonDocument js_doc(par);
            QByteArray ba_doc = js_doc.toJson();
            qDebug() << "i_collection::__to_file(): json " << ba_doc.left(32) << "...";

            f_def.write(ba_doc);
            return true;
        }

        qDebug() << "i_collection::__from_file(): file open error!";
        return false;
    }

protected:

    //preproces - spusten pred snimanim obrazu
    virtual void __init_measurement() = 0;

    //potprocess - zpracovani hodnot z analyz
    virtual void __eval_measurement() = 0;

    //vlastni mereni - pravdepodoben vyuzitim i_proc_stage retezce
    virtual void __proc_measurement() = 0;

    //priprava datagramu pred odeslanim - uzitim private hodnot
    virtual void __serialize_measurement_res(unsigned ord, QByteArray &to) = 0;

    //rozklad datagramu do private hodnot
    virtual void __deserialize_measurement_res(unsigned ord, QByteArray &from) = 0;

    //vola serialize intefrace abysme z aktualnich hodnot mereni a stavu vygenerovali datagram
    void reply(unsigned ord){

        QByteArray raw;
        __serialize_measurement_res(ord, raw);

        //TODO: vyskladat zobecneny paket a odeslat
        switch(ord){

            case VI_PLC_PC_TRIGGER_ACK: //meas
                log += QString("-->tx: TRIGGER\r\n");
            break;
            case VI_PLC_PC_READY:
                log += QString("-->tx: READY\r\n");
            break;
            case VI_PLC_PC_ERROR:
                log += QString("-->tx: ERROR\r\n");
            break;
            case VI_PLC_PC_BACKGROUND_ACK:
                log += QString("-->tx: BACKGROUND-ACK\r\n");
            break;
            case VI_PLC_PC_RESULT:
                log += QString("-->tx: RESULT\r\n");
            break;
        }

        iface->on_write(raw);
    }

    //vola serialize intefrace abysme si mohli data nacpat do nejakych globalnich hodnot
    void receive(unsigned ord, QByteArray raw){

        __deserialize_measurement_res(ord, raw);

        log.clear();

        switch(ord){

            case VI_PLC_PC_TRIGGER: //meas
                log += QString("<--rx: TRIGGER\r\n");
            break;
            case VI_PLC_PC_ABORT:
                log += QString("<--rx: ABORT\r\n");
            break;
            case VI_PLC_PC_READY:
                log += QString("<--rx: READY\r\n");
            break;
            case VI_PLC_PC_ERROR:
                log += QString("<--rx: ERROR\r\n");
            break;
            case VI_PLC_PC_BACKGROUND:
                log += QString("<--rx: BACKGROUND\r\n");
            break;
            case VI_PLC_PC_CALIBRATE:
                log += QString("<--rx: CALIBRATE\r\n");
            break;
        }
    }

public slots:

    //defaultni chovani na standarni akce
    virtual int on_done(int res, void *img){

        res = res;
        img = img;
        return 0;
    }

    //slot co se zavola s prijmem povelu od plc
    virtual int on_order(unsigned ord, QByteArray raw){

        etimer.start();

        receive(ord, raw);

        switch(ord){

            case VI_PLC_PC_TRIGGER: //meas
            {
                store.increment();

                error_mask = VI_ERR_OK;

                //potvrdime prijem
                reply(VI_PLC_PC_TRIGGER_ACK); //tx ack to plc

                on_meas();

                reply(VI_PLC_PC_RESULT);  //tx results to plc
            }
            break;
            case VI_PLC_PC_ABORT:
                on_abort(); //nastavi preruseni a ceka na jeho vyhodnoceni
                //a prekontrolujem jak na tom sme
            break;
            case VI_PLC_PC_READY:
            {
                if(on_ready()){
                    //potvrdime prijem
                    reply(VI_PLC_PC_READY);
                } else {
                    //nejsme operabilni
                    reply(VI_PLC_PC_ERROR);
                }
            }
            break;
            case VI_PLC_PC_BACKGROUND:
            {
                error_mask = VI_ERR_OK;
                if(on_background()){
                    //odvysilame vysledek
                    reply(VI_PLC_PC_BACKGROUND_ACK);
                }
            }
            break;
            case VI_PLC_PC_CALIBRATE:
            {
                store.increment();

                error_mask = VI_ERR_OK;

                on_calibration();

                reply(VI_PLC_PC_CALIBRATE_ACK);
            }
            break;
        }

        log += QString("command time %1ms\r\n").arg(etimer.elapsed());
        store.append(log);

        return 0;
    }

    //odpaleni snimani analyza a odreportovani vysledku
    //volame zatim rucne
    virtual int on_trigger(){

        qDebug() << "i_collection::on_trigger(): enter";

        const int img_reserved = 3000 * 2000;
        if(img){ delete[] img; img = NULL; }

        img = (uint8_t *) new uint8_t[img_reserved];
        if(!img){

            error_mask |= VI_ERR_CAM_MEMORY;
            qDebug() << "i_collection::on_trigger():  alocation failed!";
            log += QString("cam-error: alocation failed!");
            return 0;
        }

        //acquisition
        int pisize = 0;
        for(int rep = 0; pisize <= 0; rep++){

            if(cam_device.sta == i_vi_camera_base::CAMSTA_PREPARED){

                qDebug() << "i_collection::on_trigger(): cam_device.snap";
                pisize = cam_device.snap(img, img_reserved, &info);
            } else {

                qDebug() << "i_collection::on_trigger(): cam_simul.snap";
                error_mask |= VI_ERR_CAM_NOTFOUND;
                pisize = cam_simul.snap(img, img_reserved, &info);
            }

            if((rep >= 5) || abort){

                error_mask |= VI_ERR_CAM_TIMEOUT;

                switch(pisize){

                    case 0:     error_mask |= VI_ERR_CAM_SNAPERR;   break;
                    case -101:  error_mask |= VI_ERR_CAM_BADPICT;   break;
                    case -102:  error_mask |= VI_ERR_CAM_EXCEPTION; break;
                }

                qDebug() << "i_collection::on_trigger(): snap error/timeout - error mask" << error_mask;
                log += QString("cam-error: timeout / abort");
                abort = false;
                return 0;
            }

            QEventLoop loop;  //process pottential abort
            loop.processEvents();
        }

        if(info.w * info.h <= 0){

            error_mask |= VI_ERR_CAM_BADPICT;
            qDebug() << "i_collection::on_trigger(): bad picture" << info.w << info.h;
            log += QString("cam-error: bad picture");
            return 0;
        }

        qDebug() << "i_collection::on_trigger(): picture" << info.w << info.h;
        return 1;
    }

    //obsahuje snimani obrazu a analyzu
    virtual int on_meas(void){

        qDebug() << "i_collection::on_meas(): enter";
        __init_measurement();

        //v ramci on_trigger se vola i __proc_measurement()
        int res = on_trigger();
        if(!res) res = on_trigger(); //opakujem 2x pokud se mereni nepovede

        if(res){

            QVector<QRgb> colorTable;
            for (int i = 0; i < 256; i++)
                colorTable.push_back(QColor(i, i, i).rgb());

            snapshot = QImage(img, info.w, info.h, QImage::Format_Indexed8);
            snapshot.setColorTable(colorTable);

            store.insert(snapshot);
            emit present_preview(snapshot, 0, 0);    //vizualizace preview kamery

            //process measurement
            ptimer.start();
            __proc_measurement();
            log += QString("analysis time %1ms\r\n").arg(ptimer.elapsed());

            //prepocet pixelu na mm nebo cokoli jineho
            __eval_measurement();
        } else {

            QImage output(":/error-498-fix.gif");
            emit present_meas(output, -1, -1);
        }

        return res;
    }

    virtual int on_abort(){

        qDebug() << "i_collection::on_abort(): enter";

        abort = true;

        /*! \todo - qloop a cekame na vyhodnoceni (smazani abortu) */
        return 1;
    }

    virtual int on_ready(){

        qDebug() << "i_collection::on_ready(): enter";

        error_mask = VI_ERR_OK;
        if(cam_device.sta != i_vi_camera_base::CAMSTA_PREPARED){

            error_mask |= VI_ERR_CAM_NOTFOUND;
            return 0;
        }

        return 1;
    }

    virtual int on_background(){

        qDebug() << "i_collection::on_background(): enter";
        return 1;
    }

    virtual int on_calibration(){

        qDebug() << "i_collection::on_calibration(): enter";
        return on_trigger();
    }

signals:
    void process_next(int p1, void *p2);  //pro vstupovani mezi jednotlive analyzy - see intercept() slot

    void present_meas(QImage &im, double p1, double p2);  //vizualizace mereni
    void present_preview(QImage &im, double p1, double p2);    //vizualizace preview kamery

public slots:

    int intercept(int p1, void *p2){

        /*! place some data manipulation between
         * two t_vi_proc_* stages
         */

        emit process_next(p1, p2);
        return 1;
    }

public:

    int initialize(){

        qDebug() << "i_collection::initialize()";
        cam_device.init();
        cam_simul.init();
        return 1;
    }

    i_collection(QString &js_config,
                 QString &pt_storage,
                 i_comm_generic *comm,
                 QObject *parent = NULL):
        QObject(parent),
        path(js_config),
        par(__from_file()),
        cam_device(path),
        cam_simul(path),
        abort(false),
        iface(comm),
        store(pt_storage)
    {

        qDebug() << "i_collection::i_collection()";

        //z vnejsu vyvolana akce
        QObject::connect(iface, SIGNAL(order(unsigned, QByteArray)), this, SLOT(on_order(unsigned, QByteArray)));

        //init
        img = NULL;
    }

    virtual ~i_collection(){
        if(img) delete[] img;
    }
};

#endif // I_COLLECTION

