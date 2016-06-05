QT += core
QT += network
QT -= gui

TARGET = vi_interface
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += vi_interface_main.cpp

include(vi_interface.pri)

HEADERS += \
    t_comm_dgram_intev.h

