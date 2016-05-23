#include <QApplication>
#include <QProcessEnvironment>
#include <QDebug>
#include <QLabel>

#include <stdio.h>
#include "mainwindow.h"

#include "t_inteva_app.h"
#include "t_vi_specification.h"

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

    MainWindow w;
    w.show();

    t_comm_tcp_inteva comm(9199);  //todo - read from config or postpone settings to t_collection

    QString config_path = QDir::currentPath() + "/config.txt";
    t_inteva_app worker(comm, config_path);
    worker.initialize();

    QObject::connect(&worker, SIGNAL(present_meas(QImage &,double,double)),  //vizualizace mereni
                     &w, SLOT(measured(QImage&,double,double)));

    QObject::connect(&worker, SIGNAL(present_preview(QImage &,double,double)),    //vizualizace preview kamery
                     &w, SLOT(preview(QImage&,double,double)));

    QObject::connect(&w, SIGNAL(on_trigger_button()),
                     &worker, SLOT(on_meas()));  //snimek & analyza

    QObject::connect(&w, SIGNAL(on_background_button()),
                     &worker, SLOT(on_background()));  //novy snimek pozadi

    QObject::connect(&w, SIGNAL(on_ready_button()),
                     &worker, SLOT(on_ready()));  //kontrola kamery & nastaveni expozice pokud je podporovano

    return a.exec();
}

