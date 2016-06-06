#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "tcp_uni/t_comm_tcp_uni.h"
#include "t_comm_dgram_intev.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    t_comm_dgram_intev dgram;
    t_comm_parser_binary_ex parser;
    t_comm_tcp control;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void on_trigger();
    void on_ready();
    void on_abort();
    void on_calibration();
    void on_background();
    void on_ack();

    void on_receive(uint8_t ord, QByteArray par);

private:
    Ui::MainWindow *ui;
    QTimer ticker;
};

#endif // MAINWINDOW_H
