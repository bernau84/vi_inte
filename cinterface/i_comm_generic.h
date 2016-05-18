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

#define VI_COMM_REFRESH_RT  50  //definuje reakcni cas

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

        event = event;
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

        ord = ord;

        QString rawpar;
        for(int i=0; i < par.size(); i++)
            rawpar += QString("0x%1(%2),").arg(uint8_t(par[i]), 2, 16, QChar('0')).arg(QChar((uint8_t(par[i]) > 32) ? par[i] : '?'));

        //qDebug() << "ord" << QString::number(ord) << ": " << rawpar;
    }

    int refresh(){

        QByteArray dt;
        on_read(dt);

        int ret = -2; //== cekame na konec radky
        if(dt.isEmpty() == false){

            for(int i=0; i<dt.length(); i++)
                if((ret = parser->feed(dt[i])) >= 0){

                    std::vector<uint8_t> st_ord = parser->getlast();
                    QByteArray qt_ord;

                    for(std::vector<uint8_t>::const_iterator i = st_ord.begin(); i < st_ord.end(); i++)
                        qt_ord.append(*i);

                    callback(ret, qt_ord);
                    emit order(ret, qt_ord);
                }
        }

        return ret;
    }

    void query_command(QByteArray &cmd, int timeout){

        on_write(cmd);
        while(timeout > 0){

            if(refresh() >= 0)
                break;

            QMutex localMutex;
            localMutex.lock();
            QWaitCondition sleepSimulator;
            sleepSimulator.wait(&localMutex, 10);
            localMutex.unlock();

            timeout -= 10;
        }
    }

    i_comm_generic(i_comm_parser *_parser, QObject *parent = NULL) :
        parser(_parser),
        QObject(parent)
    {
        sta = COMMSTA_UNKNOWN;

        if(VI_COMM_REFRESH_RT)
            this->startTimer(VI_COMM_REFRESH_RT);
    }

    virtual ~i_comm_generic(){

    }
};


#endif // I_COMM_GENERIC

