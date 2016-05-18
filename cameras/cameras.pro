#-------------------------------------------------
#
# Project created by QtCreator 2015-10-07T18:45:32
#
#-------------------------------------------------

QT       += core gui
QT       += widgets

TARGET = cameras2

TEMPLATE = app

INCLUDEPATH += "c:\Program Files\Basler\pylon 4\pylon\include"
INCLUDEPATH += "c:\Program Files\Basler\pylon 4\genicam\library\CPP\include"

LIBS += -L"c:\Program Files\Basler\pylon 4\pylon\lib\Win32"
LIBS += -L"c:\Program Files\Basler\pylon 4\genicam\library\CPP\Lib\Win32_i86"

INCLUDEPATH += "C:\\opencv\\build\\include"

LIBS += -L"C:\\opencv\\build\\x86\\vc10\\lib" \
    -lopencv_core2410d \
    -lopencv_highgui2410d \
    -lopencv_imgproc2410d \
    -lopencv_features2d2410d \
    -lopencv_calib3d2410d

SOURCES += main.cpp

HEADERS += \
    i_camera_base.h \
    t_camera_attrib.h \
    ../t_vi_setup.h \
    basler/t_vi_camera_basler_usb.h \
    offline/t_vi_camera_offline_file.h \
    usbweb/t_vi_camera_web_usb.h \
    basler/t_vi_camera_basler_gige.h

DISTFILES += \
    js_camera_base.txt

DEFINES += USE_USB
#DEFINES += USE_GIGE
