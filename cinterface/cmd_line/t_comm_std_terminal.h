#ifndef T_VI_COMM_STD_TERMINAL
#define T_VI_COMM_STD_TERMINAL

#include "../t_comm_parser_string.h"
#include "../i_comm_generic.h"
#include <iostream>

class t_comm_stdte : public i_comm_generic {

    Q_OBJECT

private:
    t_comm_parser_string parser;

public:
    virtual void on_read(QByteArray &dt){

        if(std::cin.rdbuf()->in_avail() == 0)
            return;

        char c = std::cin.get();  //v terminalu qt creatoru + win to zrovna nefunguje, ani cin.readsome(&c, 1)
        if(c) dt.append(c);
    }

    virtual void on_write(QByteArray &dt){

        std::cout << dt.toStdString();
    }

    t_comm_stdte (const char *orders[] = NULL, QObject *parent = NULL):
        parser(orders),
        i_comm_generic(&parser, parent)
    {

    }

    ~t_comm_stdte (){

    }
};

#endif // T_VI_COMM_STD_TERMINAL

