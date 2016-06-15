#include <QCoreApplication>
#include "cmd_line/t_comm_std_terminal.h"
#include "tcp_uni/t_comm_tcp_uni.h"
#include "t_comm_dgram_intev.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    t_comm_dgram_intev dgram;
    t_comm_parser_binary_ex parser(dgram);
    t_comm_tcp serv(9199, &parser);

    QEventLoop loop;
    while(serv.health() != COMMSTA_PREPARED){

        loop.processEvents();
    }

    memset(&dgram.d, 0, sizeof(dgram.d));
    dgram.d.sync[0] = sync_pattern[0];
    dgram.d.sync[1] = sync_pattern[1];
    dgram.d.sync[2] = sync_pattern[2];
    dgram.d.ord = 6;

//    QByteArray ba((char *)&dgram.d, sizeof(dgram.d));
//    serv.query(ba, 100);

//    //dgram.ord = 2;
//    //loc_cli.query_command(QByteArray((char *)&dgram, sizeof(dgram)), 100);

//    dgram.ord = 3;
//    rem_serv.query_command(QByteArray((char *)&dgram, sizeof(dgram)), 100);
//    //dgram.ord = 4;
//    //loc_cli.query_command(QByteArray((char *)&dgram, sizeof(dgram)), 100);


    return a.exec();
}

