
#include <QApplication>
#include <QProcessEnvironment>
#include <QDebug>
#include <QLabel>

#include <stdio.h>

#include "basler/t_vi_camera_basler_usb.h"
#include "offline/t_vi_camera_offline_file.h"
#include "basler/t_vi_camera_basler_gige.h"
#include "usbweb/t_vi_camera_web_usb.h"

int main(int argc, char *argv[])
{
    /*
        @Echo Off
        REM Enable pylon logging for this run. The logfile will be created in the %TEMP% directory and named pylonLog.txt
        REM You can drag-n-drop any application from the explorer onto this script file to run it with logging enabled.
        REM If you just start this script with no args the pylonViewer will be started.

        IF !%PYLON_GENICAM_VERSION%! == !! SET PYLON_GENICAM_VERSION=V2_3
        SET GENICAM_LOG_CONFIG_%PYLON_GENICAM_VERSION%=%PYLON_ROOT%\..\DebugLogging.properties

        REM start the application

        ECHO Logging activated
        ECHO Waiting for application to exit ...

        IF !%1! == !! (
            Start /WAIT "PylonViewerApp" PylonViewerApp.exe
        ) ELSE (
            Start /D"%~dp1" /WAIT "%~n1" %1
        )

        rem open an explorer window and select the logfile
        IF EXIST "%TEMP%\pylonLog.txt" Start "" explorer.exe /select,%TEMP%\pylonLog.txt
    */

    QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();

    QString genicam_key = "PYLON_GENICAM_VERSION";
    QString genicam_value = "V2_3";
    qDebug() << "QProcessEnvironment Insert" << genicam_key << genicam_value;
    pe.insert(genicam_key, genicam_value);

    QString log_config_key = "GENICAM_LOG_CONFIG_";
    log_config_key += genicam_value;

    QString log_config_value = pe.value("PYLON_ROOT");
    log_config_value += pe.value("\\..\\DebugLogging.properties");

    qDebug() << "QProcessEnvironment Insert" << log_config_key << log_config_value;
    pe.insert(log_config_key, log_config_value);

    QApplication a(argc, argv);

    //t_vi_camera_basler_usb dev;
//    t_vi_camera_basler_gige deve;
    t_vi_camera_offline_file simul;
//    t_vi_camera_web_usb inbuild;

//    deve.init();
    simul.init();
//    inbuild.init();

    uint8_t *img = (uint8_t *) new uint8_t[4000 * 3000];
    i_vi_camera_base::t_campic_info info;

//    simul.snap(img, 4000 * 3000 * 4, &info);

//    QLabel vizual1;
//    vizual1.setPixmap(QPixmap::fromImage(QImage(img, info.w, info.h, (QImage::Format)info.format)));
//    vizual1.show();

    //deve.snap(img, 4000 * 3000, &info);

    //inbuild.snap(img, 4000 * 3000 * 4, &info);

    QLabel vizual2;
    vizual2.setPixmap(QPixmap::fromImage(QImage(img, info.w, info.h, (QImage::Format)info.format)));
    vizual2.show();

    return a.exec();
}
