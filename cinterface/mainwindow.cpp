#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    parser(dgram),
    control(QUrl("http://localhost:9199"), &parser, this),
    ticker()
{
    ui->setupUi(this);

    connect(ui->triggerButton, SIGNAL(clicked()), this, SLOT(on_trigger()));
    connect(ui->errorButton, SIGNAL(clicked()), this, SLOT(on_error()));
    connect(ui->readyButton, SIGNAL(clicked()), this, SLOT(on_ready()));
    connect(ui->calibrButton, SIGNAL(clicked()), this, SLOT(on_calibration()));
    connect(ui->backButton, SIGNAL(clicked()), this, SLOT(on_background()));
    connect(ui->ackButton, SIGNAL(clicked()), this, SLOT(on_ack()));

    connect(&control, SIGNAL(order(uint8_t,QByteArray)), this, SLOT(on_receive(uint8_t,QByteArray)));

    ticker.start(1000);
    connect(&ticker, SIGNAL(timeout()), this, SLOT(on_ready()));
}

void MainWindow::on_trigger(){

    static int trig_no = 0;

    t_comm_dgram_intev::stru_inteva_dgram stru = {
        sync_pattern[0], sync_pattern[1], sync_pattern[2],
        VI_ORD_TRIGGER,
        0,
        1000, 999, 10,
        trig_no++
    };

    QByteArray dt((const char *)&stru, sizeof(stru));
    control.on_write(dt);
}

void MainWindow::on_calibration(){

}

void MainWindow::on_background(){

}

void MainWindow::on_ack(){

    t_comm_dgram_intev::stru_inteva_dgram stru = {
        sync_pattern[0], sync_pattern[1], sync_pattern[2],
        VI_ORD_RESULT_ACK,
        0,
        1000, 999, 10,
        0
    };

    QByteArray dt((const char *)&stru, sizeof(stru));
    control.on_write(dt);
}

void MainWindow::on_abort(){

    t_comm_dgram_intev::stru_inteva_dgram stru = {
        sync_pattern[0], sync_pattern[1], sync_pattern[2],
        VI_ORD_ABORT,
        0,
        1000, 999, 10,
        0
    };

    QByteArray dt((const char *)&stru, sizeof(stru));
    control.on_write(dt);
}

void MainWindow::on_ready(){


    t_comm_dgram_intev::stru_inteva_dgram stru = {
        sync_pattern[0], sync_pattern[1], sync_pattern[2],
        VI_ORD_READY,
        0,
        1000, 999, 10,
        0
    };

    QByteArray dt((const char *)&stru, sizeof(stru));
    control.on_write(dt);
}

void MainWindow::on_receive(uint8_t ord, QByteArray par){

    qDebug() << "control received order #" << ord;
}


MainWindow::~MainWindow()
{
    delete ui;
}
