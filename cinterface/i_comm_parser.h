#ifndef I_COMM_PARSER
#define I_COMM_PARSER

#include <stdint.h>
#include <string.h>

class i_comm_parser {

protected:
    std::vector<uint8_t> tmp;
    std::vector<uint8_t> last;

public:
    //nova data pro parser
    //>=0 cislo povelu - zavisle na implementaci
    //-1 povel neexistuje
    //-2 cekame na konec (u stringu radky, u binary na data pro datagram)
    virtual int feed(uint8_t p) = 0;

public:
    //vraci posledni raw paket
    std::vector<uint8_t> getlast(){

        return last;
    }

    i_comm_parser(){;}
    virtual ~i_comm_parser(){;}
};

#endif // I_COMM_PARSER

