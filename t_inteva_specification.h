#ifndef T_VI_SPECIFICATION
#define T_VI_SPECIFICATION

#include "cinterface\tcp_uni\t_comm_tcp_uni.h"
#include "cinterface\t_comm_parser_binary.h"

//vyznam bitu definuje dokument "datagram-pl-pc-cpecification"
//https://docs.google.com/document/d/1mEbkwBnB_EEMSWxB8eQIjCiENeP74lsUJXaSv8vnYG8/edit

//const char *s_vi_plc_pc_ords[] = {
//    "MEAS", "ABORT", "OK", "RESULT", NULL
//};

//puvodne to byly ciala ale jakubneu to pochopil jako masku - budiz tedy
enum e_vi_pc_ords {

    VI_PLC_PC_TRIGGER = (0 << 0),
    VI_PLC_PC_ABORT = (1 << 0),
    VI_PLC_PC_RESULT = (2 << 0),
    VI_PLC_PC_TRIGGER_ACK = (3 << 0),
    VI_PLC_PC_RESULT_ACK = (4 << 0),
    VI_PLC_PC_ERROR = (5 << 0), //see e_vi_plc_pc_errors
    VI_PLC_PC_READY = (6 << 0),
    VI_PLC_PC_CALIBRATE = (7 << 0),
    VI_PLC_PC_CALIBRATE_ACK = (8 << 0),
    VI_PLC_PC_BACKGROUND = (9 << 0),
    VI_PLC_PC_BACKGROUND_ACK = (10 << 0)
};


enum e_vi_pc_errors {

    VI_ERR_OK = (1 << 0),
    VI_ERR_COMM_SYNTAX = (1 << 1),
    VI_ERR_COMM_TIMEOUT = (1 << 2),      //na prijem kompletniho datagramu mysleno

    VI_ERR_PLC1 = (1 << 3),
    VI_ERR_PLC2 = (1 << 4),
    VI_ERR_PLC3 = (1 << 5),

    VI_ERR_CAM_NOTFOUND = (1 << 10),
    VI_ERR_CAM_INITERR = (1 << 11),
    VI_ERR_CAM_TIMEOUT = (1 << 12),
    VI_ERR_CAM_BADPICT = (1 << 13),
    VI_ERR_CAM_EXCEPTION = (1 << 14),
    VI_ERR_CAM_SNAPERR = (1 << 15),
    VI_ERR_CAM_EXPOSITION = (1 << 16),
    VI_ERR_CAM_MEMORY = (1 << 17),

    VI_ERR_MEAS1 = (1 << 20),
    VI_ERR_MEAS2 = (1 << 21),
    VI_ERR_MEAS3 = (1 << 22),
    VI_ERR_MEAS4 = (1 << 23),
    VI_ERR_MEAS5 = (1 << 24),
    VI_ERR_MEAS6 = (1 << 25),
    VI_ERR_MEAS7 = (1 << 26),
    VI_ERR_MEAS8 = (1 << 27),
    VI_ERR_MEAS9 = (1 << 28)
};


#pragma pack(push,1)
typedef struct {

    uint8_t sync[3];
    uint8_t ord;
    uint32_t flags;
    uint32_t leftp;
    uint32_t rightp;
    uint32_t gap;
    uint32_t count;
} stru_inteva_dgram;
#pragma pack(pop)

const char sync_inveva_pattern[] = "\x55\xFF\xAA";

class t_comm_dgram_intev : public i_comm_dgram {

private:
    e_comm_parser_res sta;

public:

    stru_inteva_dgram d;

    virtual e_comm_parser_res validate(std::vector<uint8_t> &stream){

        if(stream.size() < sizeof(stru_inteva_dgram))
            return ((sta = ECOMM_PARSER_WAITING_ENDOFORD));

        std::copy(stream.begin(), stream.end(), (uint8_t *)&d);  //todo: hlasi warningy; asi lepsi predelat na for cyklus

        if(memcmp(sync_inveva_pattern, d.sync, 3))
            return ((sta = ECOMM_PARSER_WAITING_SYNC));

        return ((sta = (e_comm_parser_res)d.ord));
    }

    bool is_valid(){

        return (sta >= ECOMM_PARSER_MATCH_ORDNO_0);
    }
};

//verze tcp parseru a interface specificka pro ucely projektu roll-idn
//tvarime se jako tcp server a protokol je binarni
class t_comm_tcp_inteva : public t_comm_tcp {

private:
    t_comm_dgram_intev dgram;
    t_comm_parser_binary_ex parser;

public:
    t_comm_tcp_inteva(uint16_t port, QObject *parent = NULL):
        parser(dgram),
        t_comm_tcp(port, &parser, parent)
    {
    }

    virtual ~t_comm_tcp_inteva(){

    }

};

#endif // T_VI_SPECIFICATION

