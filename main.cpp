#include <QApplication>
#include <QProcessEnvironment>
#include <QDebug>
#include <QLabel>
#include <QDateTime>

#include <stdio.h>
#include "mainwindow.h"

#include "t_inteva_app.h"
#include "t_inteva_specification.h"

QFile f_debug;

void m_debug_msg_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);

    QString datetime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    f_debug.write("[", 1);
    f_debug.write(datetime.toLocal8Bit().data(), datetime.length());
    f_debug.write("] ", 2);
    f_debug.write(msg.toLocal8Bit().data(), msg.size());
    f_debug.write("\n", 1);
    f_debug.flush();
}

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

    /*
    QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();

    QString genicam_key = "PYLON_GENICAM_VERSION";
    QString genicam_value = "V2_3";
    qDebug() << "main():" << "QProcessEnvironment Insert" << genicam_key << genicam_value;
    pe.insert(genicam_key, genicam_value);

    QString log_config_key = "GENICAM_LOG_CONFIG_";
    log_config_key += genicam_value;

    QString log_config_value = pe.value("PYLON_ROOT");
    log_config_value += pe.value("\\..\\DebugLogging.properties");

    qDebug() << "main():" << "QProcessEnvironment Insert" << log_config_key << log_config_value;
    pe.insert(log_config_key, log_config_value);
    */

    // redirect terminal output
    QDateTime tmstamp = QDateTime::currentDateTime();
    QString debug_path = QDir::currentPath() + "/debug" + tmstamp.toString("yyMMddhhmmss") + ".log";
    f_debug.setFileName(debug_path);
    f_debug.open(QIODevice::WriteOnly | QIODevice::Text);
    qInstallMessageHandler(m_debug_msg_handler);

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    // default config
    QString config_path = QDir::currentPath() + "/config.txt";
    QFile f_def(config_path);  //from resources
    f_def.open(QIODevice::ReadOnly | QIODevice::Text);

    QJsonDocument js_doc = QJsonDocument::fromJson(f_def.readAll());
    t_vi_setup config(js_doc.object());
    int port = config["tcp-server-port"].get().toInt();
    QString storage_path = config["storage-path"].get().toString();

    qDebug() << "main():" << "config-path" << config_path;
    qDebug() << "main():" << "storage-path" << storage_path + "/" + RECORD_PATTERN_CNF;

    t_comm_tcp_inteva comm(port ? port : 9199);  //todo - read from config or postpone settings to t_collection
    t_inteva_app worker(comm, config_path, storage_path);
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

