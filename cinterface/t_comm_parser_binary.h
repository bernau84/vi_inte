#ifndef T_COMM_PARSER_DATAGRAM
#define T_COMM_PARSER_DATAGRAM

#include <stdio.h>
#include <stdint.h>

#include "i_comm_parser.h"
#include "i_comm_generic.h"

/*! examples
#pragma pack(push,1)
struct t_comm_binary_te1 {

    uint16_t ord;
    uint32_t p[3];
};
#pragma pack(pop)

#pragma pack(push,1)
struct t_comm_binary_te2 {

    uint8_t ord;
    uint32_t p[3];
};
#pragma pack(pop)
*/


/*! \brief hloupoucky parser vyzadujici jen existenci ord pamametry ve strukture
 * neni schopen datagram nijak validovat
 */
template <typename T> class te_comm_parser_binary : public i_comm_parser {

    using i_comm_parser::tmp;
    using i_comm_parser::last;

public:

    //kod povelu predpokladame na prvni pozici
    virtual int feed(uint8_t c){

        tmp.push_back(c);
        if(tmp.size() == sizeof(T)){

            last = tmp;
            tmp.clear();

            T dg;  //docasna instance
            std::copy(last::begin(), last::end(), (uint8_t *)&dg);
            return dg.ord;  //template musi mit parametr ord!
        }

        return ECOMM_PARSER_WAITING_ENDOFORD;
    }

public:
    te_comm_parser_binary() :
        i_comm_parser()
    {
    }

    virtual ~te_comm_parser_binary(){;}
};

/*! examples

#pragma pack(push,1)
struct t_comm_binary_te2 {
    uint8_t sync[3];
    uint8_t ord;
    uint32_t p[3];
};
#pragma pack(pop)
*/

/*! \brief hloupoucky parser vyzadujici jen existenci ord pamametry ve strukture
 * neni schopen datagram nijak validovat
 */
template <typename T> class te_comm_syncedparser_binary : public i_comm_parser {

    using i_comm_parser::tmp;
    using i_comm_parser::last;

private:
    const char *pattern;

public:

    //kod povelu predpokladame na prvni pozici
    virtual int feed(uint8_t c){

        tmp.push_back(c);
        if(tmp.size() == sizeof(T)){

            T dg;  //docasna instance
            std::copy(tmp::begin(), tmp::end(), (uint8_t *)&dg);
            if(memcmp(dg.sync, pattern, sizeof(dg.sync))){  //teplate musi mit i sync pole

                tmp.erase(tmp.begin(),tmp.begin());
                return ECOMM_PARSER_WAITING_SYNC;
            }

            last = tmp;
            tmp.clear();
            return dg.ord;  //template musi mit parametr ord!
        }

        return ECOMM_PARSER_WAITING_ENDOFORD;
    }

public:
    te_comm_syncedparser_binary(const char *_pattern) :
        i_comm_parser(),
        pattern(_pattern)
    {
    }

    virtual ~te_comm_syncedparser_binary(){;}
};


#endif // T_COMM_PARSER_DATAGRAM

