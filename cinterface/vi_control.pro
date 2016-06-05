#-------------------------------------------------
#
# Project created by QtCreator 2015-12-28T21:32:06
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vi_control
TEMPLATE = app

include(vi_interface.pri)

SOURCES += vi_control_main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h
FORMS    += mainwindow.ui
