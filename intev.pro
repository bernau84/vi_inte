QT += core gui
QT += widgets
QT += network

TARGET = intev
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

VERSION = 1.2.7.6

SOURCES += main.cpp \
    mainwindow.cpp

INCLUDEPATH += "c:\Program Files\Basler\pylon 4\pylon\include"
INCLUDEPATH += "c:\Program Files\Basler\pylon 4\genicam\library\CPP\include"

LIBS += -L"c:\Program Files\Basler\pylon 4\pylon\lib\Win32"
LIBS += -L"c:\Program Files\Basler\pylon 4\genicam\library\CPP\Lib\Win32_i86"


INCLUDEPATH += "C:\\opencv\\build\\include"

#kvuli releasu jsou to nedebugove knihovny
#mozna kvuli tomu release padal, mozna chybely nakopirovat do system32
#podezreni na chybu win R6034
LIBS += -L"C:\\opencv\\build\\x86\\vc10\\lib" \
    -lopencv_core2410 \
    -lopencv_highgui2410 \
    -lopencv_imgproc2410 \
    -lopencv_features2d2410 \
    -lopencv_calib3d2410

SUBDIRS += \
    cameras/cameras.pro \
    cinterface/cinterface.pro \
    processing/processing.pro

DISTFILES += \
    cameras/js_camera_base.txt \
    processing/js_config_cylinder_approx.txt \
    processing/js_config_roi_colortransf.txt \
    processing/js_config_threshold_cont.txt \
    js_config_collection_all.txt \
    processing/js_config_rectification.txt \
    config.txt

HEADERS += \
    cameras/i_camera_base.h \
    cameras/t_camera_attrib.h \
    t_vi_setup.h \
    cameras/basler/t_vi_camera_basler_usb.h \
    cameras/offline/t_vi_camera_offline_file.h \
    t_record_storage.h \
    cinterface/i_comm_generic.h \
    cinterface/i_comm_parser.h \
    cinterface/t_comm_parser_binary.h \
    cinterface/t_comm_parser_string.h \
    cinterface/cmd_line/t_comm_std_terminal.h \
    cinterface/tcp_uni/t_comm_tcp_uni.h \
    mainwindow.h \
    processing/t_vi_proc_roi_colortransf.h \
    processing/t_vi_proc_statistic.h \
    processing/i_proc_stage.h \
    processing/t_vi_proc_threshold_cont.h \
    cameras/basler/t_vi_camera_basler_gige.h \
    t_inteva_app.h \
    i_collection.h \
    cinterface/t_comm_parser_binary_ex.h \
    t_inteva_specification.h \
    processing/t_vi_proc_fitline.h

DEFINES += USE_USB
#DEFINES += USE_GIGE

RESOURCES += \
    processing/defaults.qrc \
    graphics.qrc

FORMS += \
    mainwindow.ui

