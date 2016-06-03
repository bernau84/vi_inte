#include <QCoreApplication>
#include "cmd_line/t_comm_std_terminal.h"
#include "tcp_uni/t_comm_tcp_uni.h"

class t_inteva_dgram : i_comm_dgram {

private:
    e_comm_parser_res sta;

public:

#pragma pack(push,1)
    struct stru_inteva_dgram {

        uint8_t sync[3];
        uint8_t ord;
        uint32_t flags;
        uint32_t leftp;
        uint32_t rightp;
        uint32_t gap;
        struct {
            uint32_t size;
            uint8_t p[1];
        } opt;
    };
#pragma pack(pop)

    virtual e_comm_parser_res validate(std::vector<uint8_t> &stream){

        if(stream.size() != sizeof(stru_inteva_dgram))
            return ((sta = ECOMM_PARSER_WAITING_ENDOFORD));

        if(sync[0] != 0x55 || sync[1] != 0x55 || sync[2] != 0x55)
            return ((sta = ECOMM_PARSER_WAITING_SYNC));

        return ((sta = ord));
    }

    bool is_valid(){

        return (sta >= ECOMM_PARSER_MATCH_ORDNO_0);
    }


};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    t_inteva_dgram dgram;
    t_comm_parser_binary parser(&dgram);
    t_comm_tcp(9100, &parser);
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

