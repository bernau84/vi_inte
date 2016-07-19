#ifndef I_COMM_GENERIC
#define I_COMM_GENERIC

#include <stdio.h>
#include <stdint.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QMutex>
#include <QWaitCondition>

#include "i_comm_parser.h"

#define VI_COMM_REFRESH_RT  20  //definuje reakcni cas v ms

enum e_commsta {

    COMMSTA_UNKNOWN = 0,
    COMMSTA_PREPARED,
    COMMSTA_INPROC,
    COMMSTA_ERROR
};


class i_comm_generic : public QObject {

    Q_OBJECT

protected:
    e_commsta sta;
    i_comm_parser *parser;

private slots:
    void timerEvent(QTimerEvent *event){

        Q_UNUSED(event);
        refresh();
    }

signals:
    void order(unsigned ord, QByteArray par);

public:
    virtual void on_read(QByteArray &dt) = 0;
    virtual void on_write(QByteArray &dt) = 0;

    e_commsta health(){

        return sta;
    }

    virtual void callback(int ord, QByteArray par){

        ord = ord;  par = par;
        qDebug() << "ord" << ord << "par" << par;
    }

    e_commsta refresh(){

        QByteArray dt;
        on_read(dt);
        if(dt.isEmpty())
            return sta;

        int ret;
        for(int i=0; i<dt.length(); i++)
            switch((ret = parser->feed(dt[i]))){

                case ECOMM_PARSER_ERROR:
                    sta = COMMSTA_ERROR;
                break;
                case ECOMM_PARSER_WAITING_SYNC:
                case ECOMM_PARSER_WAITING_ENDOFORD:
                    //nic
                break;
                case ECOMM_PARSER_MISMATCH:
                    //sta = COMMSTA_UNKNOWN;
                    //TODO: trace
                break;
                default:
                case ECOMM_PARSER_MATCH_ORDNO_0:

                    std::vector<uint8_t> st_ord = parser->getlast();
                    QByteArray qt_ord((const char *)st_ord.data(), (int)st_ord.size());
                    callback(ret, qt_ord);
                    emit order(ret, qt_ord);
                break;
            }

        return sta;
    }

    void query(QByteArray &cmd, int timeout){

        on_write(cmd);

        sta = COMMSTA_INPROC;
        while((timeout > 0) && (sta == COMMSTA_INPROC)){

            refresh();

            QMutex localMutex;
            localMutex.lock();
            QWaitCondition sleepSimulator;
            sleepSimulator.wait(&localMutex, VI_COMM_REFRESH_RT);
            localMutex.unlock();

            timeout -= VI_COMM_REFRESH_RT;
        }
    }

    i_comm_generic(i_comm_parser *_parser, QObject *parent = NULL) :
        QObject(parent),
        parser(_parser)
    {
        sta = COMMSTA_UNKNOWN;

        if(VI_COMM_REFRESH_RT)
            this->startTimer(VI_COMM_REFRESH_RT);
    }

    virtual ~i_comm_generic(){

    }
};


#endif // I_COMM_GENERIC

