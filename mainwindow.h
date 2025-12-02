#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QString>
#include <QTimer>
#include <QVector>  // <-- Đã thêm thư viện QVector
#include <QDoubleValidator> // <-- Thêm thư viện này để kiểm tra số

// export data
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QDateTime> // Để lấy tên file theo ngày giờ

#include "qcustomplot.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_connect_butt_clicked();

    void init_window();

    void update_port();

    void update_baundrate();

    bool isPortConnected(const QSerialPortInfo &portInfo);

    void serialError();

    void receive_data();

    void on_stop_bt_clicked();

    void on_spid_tb_clicked();

    void on_trans_pl_bt_clicked();

    void on_rec_pl_bt_clicked();

    void plotSetting(QCustomPlot  *plot, const char* xLabel, const char * yLabel);

    void plotConfig();

    void plotRespond();

    void aliveChecking();

    void on_export_bt_clicked();

    void on_pwm_bt_clicked();

    void on_dir_bt_clicked();

private:
    Ui::MainWindow *ui;

    QSerialPort serial;
    QSerialPortInfo info;
    QTimer timer;
    QTimer check_alive_timer;
    QList<QSerialPortInfo> ports;
    bool init = true;
    bool stop = false;
    // --- ĐÃ SỬA ĐOẠN NÀY TỪ QList SANG QVector ---
    QVector<double> timeBuff, valueBuff, refBuff;
    QVector<double> atimeBuff, avalueBuff, arefBuff;
    QVector<double> aErrorBuff;

    QVector<double> errorBuff; // Vector chứa sai số


    // ---------------------------------------------
    QElapsedTimer tick_timer;
    quint64       preTime;
    float         refValue;
    QString       check_alive;
};
#endif // MAINWINDOW_H
