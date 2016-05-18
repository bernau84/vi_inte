#ifndef T_COMM_PARSER_DATAGRAM
#define T_COMM_PARSER_DATAGRAM

#include <stdio.h>
#include <stdint.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QWaitCondition>

#include "i_comm_parser.h"
#include "i_comm_generic.h"

#pragma pack(push,1)
struct t_comm_binary_template1 {

    uint16_t ord;
    uint32_t p[3];
};
#pragma pack(pop)

#pragma pack(push,1)
struct t_comm_binary_template2 {

    uint8_t ord;
    uint32_t p[3];
};
#pragma pack(pop)

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
            uint8_t *p = (uint8_t *)&dg;
            for(unsigned i=0; i<last.size(); i++)
               *p++ = last[i];

            return dg.ord;  //template musi mit parametr ord!
        }

        return -2;
    }

public:
    te_comm_parser_binary() :
        i_comm_parser()
    {
    }

    virtual ~te_comm_parser_binary(){;}
};

class t_comm_parser_bin_te1 : public te_comm_parser_binary<t_comm_binary_template1> {

public:
    t_comm_parser_bin_te1() :
        te_comm_parser_binary()
    {
    }



    virtual ~t_comm_parser_bin_te1()
    {
    }
};

class t_comm_parser_bin_te2 : public te_comm_parser_binary<t_comm_binary_template2> {

public:
    t_comm_parser_bin_te2() :
        te_comm_parser_binary()
    {
    }

    virtual ~t_comm_parser_bin_te2()
    {
    }
};

#endif // T_COMM_PARSER_DATAGRAM

