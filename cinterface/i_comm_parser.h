#ifndef I_COMM_PARSER
#define I_COMM_PARSER

#include <stdint.h>
#include <string.h>

enum e_comm_parser_res {
    ECOMM_PARSER_MATCH_ORDNO_0 = 0,     //>=0 cislo povelu - zavisle na implementaci
    ECOMM_PARSER_MISMATCH = -1,         // povel neexistuje
    ECOMM_PARSER_WAITING_ENDOFORD = -2, // cekame na konec (u stringu radky, u binary na data pro datagram)
    ECOMM_PARSER_ERROR = -3,            // invalid packet - obecne neco bylo mimo dovolene hodnoty
    ECOMM_PARSER_WAITING_SYNC = -4      // invalid packet
};

class i_comm_parser {

protected:
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> last;

public:
    //nova data pro parser
    virtual e_comm_parser_res feed(uint8_t p) = 0;

public:
    //vraci posledni raw paket
    std::vector<uint8_t> getlast(){

        return last;
    }

    i_comm_parser(){;}
    virtual ~i_comm_parser(){;}
};

#endif // I_COMM_PARSER

