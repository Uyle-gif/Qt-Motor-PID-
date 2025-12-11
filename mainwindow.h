#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#define CMD_SIZE 5
#define DATA_SIZE 25



#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QString>
#include <QTimer>
#include <QVector>
#include <QDoubleValidator>

// export data
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QDateTime>

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

    QString formatData(QString dataStr);

    void on_connect_butt_clicked();
    void on_stop_bt_clicked();
    void on_spid_bt_clicked();
    void on_trans_pl_bt_clicked();
    void on_rec_pl_bt_clicked();
    void on_export_bt_clicked();
    void on_pwm_bt_clicked();
    void on_dir_bt_clicked();

    void receive_data();
    void init_window();
    void update_port();
    void update_baundrate();
    bool isPortConnected(const QSerialPortInfo &portInfo);
    void serialError(QSerialPort::SerialPortError error);
    void plotSetting(QCustomPlot  *plot, const char* xLabel, const char * yLabel);
    void plotConfig();
    void aliveChecking();
    void update_gui_loop();

    void watchdog_timeout();
    void blink_error_loop();

private:
    Ui::MainWindow *ui;

    QSerialPort serial;
    QSerialPortInfo info;
    QTimer timer;
    QTimer *render_timer;

    QTimer *watchdog_timer;
    QTimer check_alive_timer;
    QTimer *blink_timer;


    QList<QSerialPortInfo> ports;
    bool init = true;
    bool stop = false;

    float lastFilteredValue = 0.0;

    QVector<double> timeBuff, valueBuff, refBuff;
    QVector<double> atimeBuff, avalueBuff, arefBuff;
    QVector<double> aErrorBuff;

    QVector<double> errorBuff;


    bool blink_state;
    bool is_tx_error;
    bool is_rx_error;

    void setLedColor(QLabel *label, QString text, QString color);
    void updateMotorStatusLed(bool isRunning);




    // ---------------------------------------------
    QElapsedTimer tick_timer;
    quint64       preTime;
    float         refValue;
    QString       check_alive;
};
#endif
