#ifndef T_COMM_PARSER_DATAGRAM_EX
#define T_COMM_PARSER_DATAGRAM_EX

#include <stdio.h>
#include <stdint.h>

#include "i_comm_parser.h"
#include "i_comm_generic.h"

class i_comm_dgram {

public:
    virtual e_comm_parser_res validate(std::vector<uint8_t> &stream) = 0;
    virtual ~i_comm_dgram(){}
};

/*! \brief univerzalni parser ale prenasi odpovednost na objekt datagramu
 * vyhoda je ze tahle budem podporovat vselijakou validaci (sync word, crc, rozsahy, ...)
 */
class t_comm_parser_binary_ex : public i_comm_parser {

private:
    i_comm_dgram &dg;
    using i_comm_parser::tmp;
    using i_comm_parser::last;

public:

    virtual e_comm_parser_res feed(uint8_t c){

        tmp.push_back(c);

        //docasna instance - musi mit konstuktor z std::vector
        e_comm_parser_res res = dg.validate(tmp);

        switch(res){ //musi mit fci validate() -> vychazet z i_comm_dgram

            default:
            case ECOMM_PARSER_MATCH_ORDNO_0:
                last = tmp;
                tmp.clear();
            break;
            case ECOMM_PARSER_WAITING_SYNC:
                tmp.erase(tmp.begin(), tmp.begin()); //one byte left
            break;
            case ECOMM_PARSER_WAITING_ENDOFORD:
            case ECOMM_PARSER_ERROR:
            case ECOMM_PARSER_MISMATCH:
            break;
        }

        return res;
    }

public:
    t_comm_parser_binary_ex(i_comm_dgram &_dg) :
        i_comm_parser(),
        dg(_dg)
    {
    }

    virtual ~t_comm_parser_binary_ex(){;}
};


#endif // T_COMM_PARSER_DATAGRAM_EX

