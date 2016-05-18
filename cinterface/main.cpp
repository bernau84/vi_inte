#include <QCoreApplication>
#include "cmd_line/t_comm_std_terminal.h"
#include "tcp_uni/t_comm_tcp_uni.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //t_vi_test_std_term term;
    t_comm_tcp_te2 rem_serv(9100);
    //t_comm_tcp_te2 loc_cli(QUrl("http://localhost:9100"));


    QEventLoop loop;
    while((rem_serv.health() != COMMSTA_PREPARED) /* ||
          (loc_cli.health() != COMMSTA_PREPARED) */){

        loop.processEvents();
    }

    t_comm_binary_template2 dgram;
    memset(&dgram, 0, sizeof(dgram));

    dgram.ord = 6;

    QByteArray ba((char *)&dgram, sizeof(dgram));
    rem_serv.query_command(ba, 100);
//    //dgram.ord = 2;
//    //loc_cli.query_command(QByteArray((char *)&dgram, sizeof(dgram)), 100);

//    dgram.ord = 3;
//    rem_serv.query_command(QByteArray((char *)&dgram, sizeof(dgram)), 100);
//    //dgram.ord = 4;
//    //loc_cli.query_command(QByteArray((char *)&dgram, sizeof(dgram)), 100);


    return a.exec();
}

