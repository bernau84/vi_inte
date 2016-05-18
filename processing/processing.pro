#-------------------------------------------------
#
# Project created by QtCreator 2015-10-09T08:17:16
#
#-------------------------------------------------

QT       += core

CONFIG   += console
CONFIG   -= app_bundle

TARGET = processing
TEMPLATE = app

SOURCES += main.cpp

HEADERS  += \
    i_proc_stage.h \
    ../t_vi_setup.h \
    t_vi_proc_sub_background.h \
    t_vi_proc_roi_colortransf.h \
    t_vi_proc_rectification.h \
    t_vi_proc_statistic.h \
    t_vi_proc_roll_approx.h \
    t_vi_proc_threshold_cont.h \
    t_vi_proc_fitline.h

INCLUDEPATH += "C:\\opencv\\build\\include"

LIBS += -L"C:\\opencv\\build\\x86\\vc10\\lib" \
    -lopencv_core2410d \
    -lopencv_highgui2410d \
    -lopencv_imgproc2410d \
    -lopencv_features2d2410d \
    -lopencv_calib3d2410d

#LIBS += -L"C:\\opencv\\build\\x86\\vc10\\lib" \
#    -lopencv_core2410 \
#    -lopencv_highgui2410 \
#    -lopencv_imgproc2410 \
#    -lopencv_features2d2410 \
#    -lopencv_calib3d2410

DISTFILES += \
    js_config_cylinder_approx.txt \
    js_config_roi_colortransf.txt \
    js_config_threshold_cont.txt \
    js_config_rectification.txt \
    js_config_fitline.txt

RESOURCES += \
    defaults.qrc

