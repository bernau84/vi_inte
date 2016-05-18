QT += core
QT += network
QT -= gui

TARGET = cinterface3
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

HEADERS += \
    i_comm_generic.h \
    i_comm_parser.h \
    t_comm_parser_binary.h \
    t_comm_parser_string.h \
    cmd_line/t_comm_std_terminal.h \
    tcp_uni/t_comm_tcp_uni.h

