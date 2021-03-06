#ifndef T_VI_COMM_TCP_CLI
#define T_VI_COMM_TCP_CLI

#include "../t_comm_parser_string.h"
#include "../t_comm_parser_binary_ex.h"
#include "../../t_vi_setup.h"

#include <QTcpSocket>
#include <QTcpServer>
#include <QUrl>
#include <typeinfo>

#define VI_COMM_TCPCLI_CONN_TMO 10000
#define VI_COMM_TCPPORT_DEF 51515

class t_comm_tcp : public i_comm_generic {

    Q_OBJECT

protected:
    QTcpSocket *tcp;
    QTcpServer ser;

    QUrl m_url;
    uint16_t m_port;

private slots:

    void accept(void){

        tcp = ser.nextPendingConnection();
        sta = COMMSTA_PREPARED;
        qDebug() << "t_comm_tcp::accept() Connected!";
    }

public:
    virtual void on_read(QByteArray &dt){

        if(tcp)
            if(tcp->isReadable())
                dt = tcp->readAll();

        if(dt.isEmpty() == false){

            QString ordvals; for(int i=0; i < dt.size(); i++){

                ordvals += "0x";
                ordvals += QString::number((uint8_t)(dt[i]), 16);
                if((i + 1) < dt.size()) ordvals += ",";
            }

            qDebug() << "t_comm_tcp::on_read()" << typeid(this).name() << "rx-bin: " << ordvals;
        }
    }

    virtual void on_write(QByteArray &dt){

        if(tcp)
            if(tcp->isWritable())
                tcp->write(dt);
    }

    /*
     * server mode
     */
    t_comm_tcp(uint16_t port, i_comm_parser *parser, QObject *parent = NULL):
        i_comm_generic(parser, parent),
        tcp(NULL),
        ser(),
        m_port(port)
    {

        port = m_port;
        if(port == 0) port = VI_COMM_TCPPORT_DEF;  //"random"

        if(false == ser.isListening()){

            ser.listen(QHostAddress::Any, port);
            qDebug() << "t_comm_tcp() Listen on " << port << "port";

            connect(&ser, SIGNAL(newConnection()), this, SLOT(accept()));
        }
    }

    /*
     * client mode
     */
    t_comm_tcp(QUrl &url, i_comm_parser *parser, QObject *parent = NULL):
        i_comm_generic(parser, parent),
        tcp(NULL),
        ser(),
        m_url(url)
    {
        if(m_url.isValid() == false)
            return;

        tcp = (QTcpSocket *) new QTcpSocket(this);  //parent this -> uvolnime s timto objektem

        QString host = m_url.host();
        int port = m_url.port();
        if(port == 0) port = VI_COMM_TCPPORT_DEF;

        tcp->connectToHost(host, port);
        if(tcp->waitForConnected(50000/*VI_COMM_TCPCLI_CONN_TMO*/)){

            sta = COMMSTA_PREPARED;
            qDebug() << "t_comm_tcp() Connected!";
        } else {

            tcp = NULL;
            sta = COMMSTA_ERROR;
            qDebug() << "t_comm_tcp() Connect timeout!";
        }
    }

    virtual ~t_comm_tcp(){

    }
};

#endif // T_VI_COMM_TCP_CLI

