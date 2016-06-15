#ifndef T_COMM_INTEVA_DGRAM
#define T_COMM_INTEVA_DGRAM

#include "t_comm_parser_binary_ex.h"

const char sync_pattern[] = "\x55\xFF\xAA";

class t_comm_dgram_intev : public i_comm_dgram {

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
        uint32_t count;
    } d;
#pragma pack(pop)

    virtual e_comm_parser_res validate(std::vector<uint8_t> &stream){

        if(stream.size() < sizeof(stru_inteva_dgram))
            return ((sta = ECOMM_PARSER_WAITING_ENDOFORD));

        std::copy(stream.begin(), stream.end(), (uint8_t *)&d);

        if(memcmp(sync_pattern, d.sync, 3))
            return ((sta = ECOMM_PARSER_WAITING_SYNC));

        return ((sta = (e_comm_parser_res)d.ord));
    }

    bool is_valid(){

        return (sta >= ECOMM_PARSER_MATCH_ORDNO_0);
    }
};

#endif // T_COMM_INTEVA_DGRAM

