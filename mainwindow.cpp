#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(ui->pushButton, SIGNAL(clicked(bool)), this, SIGNAL(on_ready_button()));
    QObject::connect(ui->pushButton_2, SIGNAL(clicked(bool)), this, SIGNAL(on_trigger_button()));
    QObject::connect(ui->pushButton_3, SIGNAL(clicked(bool)), this, SIGNAL(on_background_button()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::measured(QImage &im, double p1, double p2){

//#ifdef QDEBUG
    im.save("meas_original.jpg");
//#endif //QDEBUG
    QSize m(ui->canvas->width(), ui->canvas->height());
    QImage rescaled = im.scaled(m);
#ifdef QDEBUG
    rescaled.save("meas_rescaled.jpg");
#endif //QDEBUG
    ui->canvas->setPixmap(QPixmap::fromImage(rescaled));

    ui->lineEdit->setText(QString::number(p2));
    ui->lineEdit_2->setText(QString::number(p1));
}

void MainWindow::preview(QImage &im, double p1, double p2){

    QSize m(ui->preview->width(), ui->preview->height());
    QImage rescaled = im.scaled(m);
    ui->preview->setPixmap(QPixmap::fromImage(rescaled));

    ui->lineEdit->setText(QString::number(p2));
    ui->lineEdit_2->setText(QString::number(p1));
}
