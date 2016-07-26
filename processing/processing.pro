#-------------------------------------------------
#
# Project created by QtCreator 2015-10-09T08:17:16
#
#-------------------------------------------------

QT       += core

#CONFIG   += console
CONFIG   -= app_bundle

TARGET = processing
TEMPLATE = app

SOURCES += main.cpp

HEADERS  += \
    i_proc_stage.h \
    ../t_vi_setup.h \
    t_vi_proc_roi_colortransf.h \
    t_vi_proc_statistic.h \
    t_vi_proc_threshold_cont.h \
    t_vi_proc_fitline.h


INCLUDEPATH += "c:\opencv\opencv301\build\include"

#kvuli releasu jsou to nedebugove knihovny
#mozna kvuli tomu release padal, mozna chybely nakopirovat do system32
#podezreni na chybu win R6034
LIBS += -L"c:\opencv\opencv301\build\x64\vc14\lib" \
    -lopencv_world310d

#INCLUDEPATH += "c:\opencv\opencv2413\build\include"
#LIBS += -L"C:\\opencv\\opencv2413\\build\\x86\\vc10\\lib" \
#    -lopencv_core2413d \
#    -lopencv_highgui2413d \
#    -lopencv_imgproc2413d \
#    -lopencv_features2d2413d \
#    -lopencv_calib3d2413d

DISTFILES += \
    js_config_cylinder_approx.txt \
    js_config_roi_colortransf.txt \
    js_config_threshold_cont.txt \
    js_config_rectification.txt \
    js_config_fitline.txt

RESOURCES += \
    defaults.qrc

