#ifndef I_COMMUNICATION_BASE_STRING
#define I_COMMUNICATION_BASE_STRING

#include <stdio.h>
#include <stdint.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QWaitCondition>

#include "i_comm_parser.h"
#include "i_comm_generic.h"

class t_comm_parser_string : public i_comm_parser {

    using i_comm_parser::tmp;
    using i_comm_parser::last;

private:
    std::vector<const char *> orders;

public:
    virtual int feed(uint8_t p){

        if((p == '\r') || (p == '\n')){

            for(unsigned i=0; i< orders.size(); i++){

                std::string s(tmp.begin(), tmp.end());
                if(s.compare(orders[i])){

                    last = tmp;
                    tmp.clear();
                    return i;
                }
            }

            tmp.clear();
            return -1;
        }

        tmp.push_back(p);
        return -2;
    }

    //vraci kod pro registrovany povel
    unsigned reg_command(const char *ord){

        orders.push_back(ord);
        return orders.size();
    }

    t_comm_parser_string(const char *_orders[]) :
        i_comm_parser()
    {
        if(_orders)
            for(int i=0; (_orders[i]) && (*_orders[i]); i++)
                reg_command(_orders[i]);
    }

    virtual ~t_comm_parser_string(){

    }
};

#endif // I_COMMUNICATION_BASE_STRING

