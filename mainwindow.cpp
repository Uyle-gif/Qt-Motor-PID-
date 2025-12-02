#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init_window();

    timer.setInterval(1000);
    check_alive_timer.setInterval(1000);
    connect(&timer, &QTimer::timeout, this, &MainWindow::update_port);
    connect(&check_alive_timer, &QTimer::timeout, this, &MainWindow::aliveChecking);
    timer.start();
    check_alive_timer.start();

    connect(&serial, &QSerialPort::errorOccurred, this,&MainWindow::serialError);
    connect(&serial,SIGNAL(readyRead()),this,SLOT(receive_data()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


bool MainWindow::isPortConnected(const QSerialPortInfo &portInfo) {
    QSerialPort port;
    port.setPort(portInfo);
    if (port.isOpen()) {
        return true;
    }
    return false;
}

void MainWindow::serialError()
{
    if(serial.error() && serial.isOpen())
    {
        QString errorMessage = "Error: " + serial.errorString();
        QMessageBox::critical(this, "Error", errorMessage);
        on_connect_butt_clicked();
    }
}

void MainWindow::update_port()
{
    QList<QString> strPorts;
    QList<QSerialPortInfo> currentports = info.availablePorts();
    for (int i = 0; i < currentports.size(); i++) {
        const QSerialPortInfo &portInfo = currentports.at(i);
        strPorts.append(portInfo.portName());
    }
    if (currentports.size() != ports.size() || init == true )
    {
        ui->port_cb->clear();
        ui->port_cb->addItems(strPorts);
    }
    ports = currentports;
}

void MainWindow::update_baundrate()
{
    QList<qint32> baudRates = info.standardBaudRates();
    QList<QString> strBaudRates;
    for(int i = 0 ; i < baudRates.size() ; i++){
        strBaudRates.append(QString::number(baudRates.at(i)));
    }
    ui->baud_cb->addItems(strBaudRates);
}

void MainWindow::init_window()
{


    ui->connect_butt->setStyleSheet("color: #4CAF50; font-weight: bold;");
    setWindowTitle("DC MOTOR PID CONTROL SYSTEM");

    update_port();
    update_baundrate();
    ui->baud_cb->setCurrentText("9600");

    ui->dataBit_cb->clear();
    ui->dataBit_cb->addItem("5");
    ui->dataBit_cb->addItem("6");
    ui->dataBit_cb->addItem("7");
    ui->dataBit_cb->addItem("8");
    ui->dataBit_cb->setCurrentText("8");


    ui->stopBit_cb->setCurrentText("1");
    ui->parity_cb->setCurrentText("No Parity");

    ui->kp_tb->setDisabled(true);
    ui->ki_tb->setDisabled(true);
    ui->kd_tb->setDisabled(true);
    ui->sp_tb->setDisabled(true);
    ui->pwm_tb->setDisabled(true);
    ui->spid_tb->setDisabled(true);
    ui->stop_bt->setDisabled(true);
    ui->rec_pl_bt->setDisabled(true);
    ui->trans_pl_bt->setDisabled(true);
    ui->export_bt->setDisabled(true);
    ui->pwm_bt->setDisabled(true);
    ui->dir_bt->setDisabled(true);

    plotConfig();
    init = false;

    QString guide = ">= 0 only";
    ui->kp_tb->setPlaceholderText(guide);
    ui->ki_tb->setPlaceholderText(guide);
    ui->kd_tb->setPlaceholderText(guide);
    ui->sp_tb->setPlaceholderText(guide);
    ui->pwm_tb->setPlaceholderText(guide);
}


void MainWindow::receive_data()
{
    while (serial.canReadLine()) {
        QByteArray data = serial.readLine();
        QString line = QString::fromUtf8(data).trimmed();
        ui->rec_pl_tb->append(line);                // In dòng chữ ra
        ui->rec_pl_tb->moveCursor(QTextCursor::End);
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);

        if (parts.size() == 2 && parts[0] == "SPD") {
            bool ok;
            float currentSpeed = parts[1].toFloat(&ok);

            if (ok) {
                float currentError = refValue - currentSpeed;

                ui->val_txt->setText(QString::number(currentSpeed, 'f', 2));
                ui->err_txt->setText(QString::number(currentError, 'f', 2));

                double currentTimeSec = (double)tick_timer.elapsed() / 1000.0; //ms

                valueBuff.append(currentSpeed);
                refBuff.append(refValue);
                errorBuff.append(currentError);
                timeBuff.append(currentTimeSec);

                avalueBuff.append(currentSpeed);
                arefBuff.append(refValue);
                atimeBuff.append(currentTimeSec);
                aErrorBuff.append(currentError);

                if (valueBuff.size() > 10000) {
                    valueBuff.removeFirst();
                    refBuff.removeFirst();
                    errorBuff.removeFirst();
                    timeBuff.removeFirst();
                }
                plotRespond();
            }
        }
    }
}

void MainWindow::aliveChecking()
{
    // Updating
}

void MainWindow::plotSetting(QCustomPlot  *plot, const char* xLabel, const char * yLabel)
{
    QFont legendFont = font();
    legendFont.setPointSize(8);
    plot->yAxis->setLabel(yLabel);
    plot->xAxis->setLabel(xLabel);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    plot->legend->setVisible(true);
    plot->legend->setFont(legendFont);
    plot->legend->setSelectedFont(legendFont);
    plot->legend->setSelectableParts(QCPLegend::spItems);
}

void MainWindow::plotConfig()
{
    QPen penSP; penSP.setStyle(Qt::SolidLine); penSP.setWidth(2); penSP.setColor(Qt::blue);
    QPen penCurrent; penCurrent.setStyle(Qt::SolidLine); penCurrent.setWidth(2); penCurrent.setColor(Qt::red);
    QPen penError; penError.setStyle(Qt::SolidLine); penError.setWidth(2); penError.setColor(Qt::red);

    ui->plot->addGraph();
    ui->plot->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(0)->setPen(penSP);
    ui->plot->graph(0)->setName("Set point (RPM)");

    ui->plot->addGraph();
    ui->plot->graph(1)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(1)->setPen(penCurrent);
    ui->plot->graph(1)->setName("Current Value");

    plotSetting(ui->plot, "Time", "Speed (rpm)");



    ui->plotError->addGraph();
    ui->plotError->graph(0)->setPen(penError);
    ui->plotError->graph(0)->setName("Error");

    ui->plotError->xAxis->setLabel("Time");
    ui->plotError->yAxis->setLabel("Error");
    ui->plotError->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    ui->plotError->legend->setVisible(true);
    QFont legendFont = font();
    legendFont.setPointSize(8);
    ui->plotError->legend->setFont(legendFont);

    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plotError->xAxis, SLOT(setRange(QCPRange)));
    connect(ui->plotError->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis, SLOT(setRange(QCPRange)));
}

void MainWindow::plotRespond() {
    if(!stop)
    {
        ui->plot->graph(0)->setData(timeBuff, refBuff);
        ui->plot->graph(1)->setData(timeBuff, valueBuff);
        ui->plot->rescaleAxes();
        ui->plot->replot();
        ui->plot->update();

        ui->plotError->graph(0)->setData(timeBuff, errorBuff);
        ui->plotError->rescaleAxes();
        ui->plotError->replot();
        ui->plotError->update();
    }
}


void MainWindow::on_connect_butt_clicked()
{
    if (ui->connect_butt->text() == "CONNECT")
    {
        serial.setPortName(ui->port_cb->currentText());
        // để debug serial.setPortName("/dev/pts/2");
        serial.setBaudRate(ui->baud_cb->currentText().toInt());

        QString dataBits = ui->dataBit_cb->currentText();
        if(dataBits == "5") serial.setDataBits(QSerialPort::Data5);
        else if(dataBits == "6") serial.setDataBits(QSerialPort::Data6);
        else if(dataBits == "7") serial.setDataBits(QSerialPort::Data7);
        else serial.setDataBits(QSerialPort::Data8);

        QString stopBits = ui->stopBit_cb->currentText();
        if(stopBits == "1.5") serial.setStopBits(QSerialPort::OneAndHalfStop);
        else if(stopBits == "2") serial.setStopBits(QSerialPort::TwoStop);
        else serial.setStopBits(QSerialPort::OneStop);

        QString parity = ui->parity_cb->currentText();
        if(parity == "Even Parity") serial.setParity(QSerialPort::EvenParity);
        else if(parity == "Odd Parity") serial.setParity(QSerialPort::OddParity);
        else if(parity == "Space Parity") serial.setParity(QSerialPort::SpaceParity);
        else if(parity == "Mark Parity") serial.setParity(QSerialPort::MarkParity);
        else serial.setParity(QSerialPort::NoParity);

        if(serial.open(QIODevice::ReadWrite))
        {
            ui->connect_butt->setText("DISCONNECT");
            ui->connect_butt->setStyleSheet("QPushButton {color: red; font-weight: bold;}");

            ui->spid_tb->setStyleSheet("color: #4CAF50; font-weight: bold;");
            ui->stop_bt->setStyleSheet("color: #d32f2f; font-weight: bold;");
            // ui->dir_bt->setStyleSheet("font-weight: bold;");         Xấu
            // ui->pwm_bt->setStyleSheet("font-weight: bold;");
            // ui->export_bt->setStyleSheet("font-weight: bold;");

            ui->port_cb->setDisabled(true);
            ui->baud_cb->setDisabled(true);
            ui->dataBit_cb->setDisabled(true);
            ui->stopBit_cb->setDisabled(true);
            ui->parity_cb->setDisabled(true);

            ui->spid_tb->setDisabled(false);
            ui->stop_bt->setDisabled(false);
            ui->rec_pl_bt->setDisabled(false);
            ui->trans_pl_bt->setDisabled(false);
            ui->export_bt->setDisabled(false);
            ui->pwm_bt->setDisabled(false);
            ui->dir_bt->setDisabled(false);
            ui->kp_tb->setDisabled(false);
            ui->ki_tb->setDisabled(false);
            ui->kd_tb->setDisabled(false);
            ui->sp_tb->setDisabled(false);
            ui->pwm_tb->setDisabled(false);

            timer.stop();

            ui->status_lb->setText("CONNECTED: " + ui->port_cb->currentText());
            ui->status_lb->setAlignment(Qt::AlignCenter);
            ui->status_lb->setStyleSheet(" color: #4CAF50; font-weight: bold; border-radius: 5px;");

            QMessageBox::information(this, "Success", "Successfully connected to port " + ui->port_cb->currentText() + "!");
        }
        else
        {
            QString errorMessage = "Cannot open " + ui->port_cb->currentText() + "\nError: " + serial.errorString();
            QMessageBox::critical(this, "Connection Error", errorMessage);

            ui->status_lb->setText("CONNECTION FAILED");
            ui->status_lb->setAlignment(Qt::AlignCenter);
            ui->status_lb->setStyleSheet("background-color: #ff6666; color: white; font-weight: bold; border-radius: 5px;");
        }
    }
    else
    {
        serial.close();

        ui->connect_butt->setText("CONNECT");
        ui->connect_butt->setStyleSheet("QPushButton {color: green; font-weight: bold;}");

        ui->spid_tb->setStyleSheet("");
        ui->stop_bt->setStyleSheet("");
        ui->dir_bt->setStyleSheet("");
        ui->pwm_bt->setStyleSheet("");
        ui->export_bt->setStyleSheet("");

        ui->port_cb->setDisabled(false);
        ui->baud_cb->setDisabled(false);
        ui->dataBit_cb->setDisabled(false);
        ui->stopBit_cb->setDisabled(false);
        ui->parity_cb->setDisabled(false);

        ui->spid_tb->setDisabled(true);
        ui->stop_bt->setDisabled(true);
        ui->rec_pl_bt->setDisabled(true);
        ui->trans_pl_bt->setDisabled(true);
        ui->export_bt->setDisabled(true);
        ui->dir_bt->setDisabled(true);
        ui->kp_tb->setDisabled(true);
        ui->ki_tb->setDisabled(true);
        ui->kd_tb->setDisabled(true);
        ui->sp_tb->setDisabled(true);
        ui->pwm_tb->setDisabled(true);
        ui->pwm_bt->setDisabled(true);

        timer.start();

        ui->status_lb->setText("DISCONNECTED");
        ui->status_lb->setAlignment(Qt::AlignCenter);
        ui->status_lb->setStyleSheet("background-color: #cccccc; color: black; font-weight: bold; border-radius: 5px;");

        ui->statusbar->showMessage("Disconnected", 3000);
    }
}

void MainWindow::on_stop_bt_clicked()
{
    stop = true;
    QString mess = "STOP \r\n";
    serial.write(mess.toUtf8());
    ui->trans_pl_tb->append(mess);

    tick_timer.elapsed();
    preTime = 0;
}

void MainWindow::on_spid_tb_clicked()
{
    stop = false;

    refBuff.clear(); timeBuff.clear();
    valueBuff.clear(); errorBuff.clear();

    arefBuff.clear(); atimeBuff.clear();
    avalueBuff.clear(); aErrorBuff.clear();


    tick_timer.restart();

    float kpValue = ui->kp_tb->text().toFloat();
    float kiValue = ui->ki_tb->text().toFloat();
    float kdValue = ui->kd_tb->text().toFloat();
    float spValue = ui->sp_tb->text().toFloat();

    refValue = spValue;

    QString msg = "SPID " + QString::number(kpValue) + " "
                  + QString::number(kiValue) + " "
                  + QString::number(kdValue) + " "
                  + QString::number(spValue) + "\r\n";

    serial.write(msg.toUtf8());
    ui->trans_pl_tb->append(msg.trimmed());
}

void MainWindow::on_trans_pl_bt_clicked()
{
    ui->trans_pl_tb->clear();
}

void MainWindow::on_rec_pl_bt_clicked()
{
    ui->rec_pl_tb->clear();
}

void MainWindow::on_export_bt_clicked()
{
    if (atimeBuff.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No data to export!\nPlease run the motor first.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Export Data to CSV",
                                                    QDateTime::currentDateTime().toString("yyyy_MM_dd_HH_mm_ss"),
                                                    "CSV Files (*.csv)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);

        stream << "Time (s),Set Point (rpm),Actual Speed (rpm),Error\n";

        for (int i = 0; i < atimeBuff.size(); ++i) {
            double errVal = 0.0;
            if (i < aErrorBuff.size()) errVal = aErrorBuff[i];

            stream << atimeBuff[i] << ","
                   << arefBuff[i] << ","
                   << avalueBuff[i] << ","
                   << errVal << "\n";
        }

        file.close();
        ui->statusbar->showMessage("Data exported successfully: " + fileName, 5000);

    } else {
        QMessageBox::critical(this, "Error", "Cannot create file! Please check file permissions.");
    }
}

void MainWindow::on_pwm_bt_clicked()
{

    QString freqText = ui->pwm_tb->text();
    if (freqText.isEmpty()) {
        freqText = "0";
    }

    QString msg = "SFRE " + freqText + "\r\n";
    serial.write(msg.toUtf8());
    ui->trans_pl_tb->append(msg.trimmed());
}


void MainWindow::on_dir_bt_clicked()
{
    QString msg = "MDIR \r\n";
    serial.write(msg.toUtf8());
    ui->trans_pl_tb->append(msg.trimmed());
}

