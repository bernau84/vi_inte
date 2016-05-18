#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

public slots:
    void measured(QImage &im, double length, double diameter);
    void preview(QImage &im, double length, double diameter);

signals:
    void on_ready_button();
    void on_trigger_button();
    void on_background_button();
};

#endif // MAINWINDOW_H
