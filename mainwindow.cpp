#include "mainwindow.h"
#include <algorithm>
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    init_window();

    timer.setInterval(1000);
    connect(&timer, &QTimer::timeout, this, &MainWindow::update_port);
    timer.start();

    check_alive_timer.setInterval(1000);
    connect(&check_alive_timer, &QTimer::timeout, this, &MainWindow::aliveChecking);
    check_alive_timer.start();

    connect(&serial, &QSerialPort::errorOccurred, this, &MainWindow::serialError);
    connect(&serial, SIGNAL(readyRead()), this, SLOT(receive_data()));

    render_timer = new QTimer(this);
    render_timer->setInterval(33);
    connect(render_timer, &QTimer::timeout, this, &MainWindow::update_gui_loop);
    render_timer->start();

    watchdog_timer = new QTimer(this);
    watchdog_timer->setInterval(3500);
    connect(watchdog_timer, &QTimer::timeout, this, &MainWindow::on_watchdog_timeout);

    blink_timer = new QTimer(this);
    blink_timer->setInterval(500);
    connect(blink_timer, &QTimer::timeout, this, &MainWindow::blink_error_loop);
    blink_timer->start();

    is_tx_error = false;
    is_rx_error = false;
    blink_state = false;
}

QString MainWindow::formatData(QString dataStr) {
    if (dataStr.length() > DATA_SIZE) {
        dataStr = dataStr.left(DATA_SIZE);
    }
    return dataStr.leftJustified(DATA_SIZE, ' ');
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init_window()
{
    ui->connect_butt->setStyleSheet("color: #4CAF50; font-weight: bold;");
    setWindowTitle("DC MOTOR PID CONTROL SYSTEM");

    connect(ui->cb_show_log, &QCheckBox::toggled, ui->gb_debugLog, &QGroupBox::setVisible);
    ui->gb_debugLog->setVisible(false);
    ui->cb_show_log->setChecked(false);

    setLedColor(ui->lb_led_tx, "Tx", "grey");
    setLedColor(ui->lb_led_rx, "Rx", "grey");

    ui->lcd_val->setStyleSheet("background-color: white; color: #00ff00; border: 2px solid white; border-radius: 5px;");
    ui->lcd_err->setStyleSheet("background-color: white; color: #ff3333; border: 2px solid white; border-radius: 5px;");

    this->setStyleSheet(
        "QPushButton { font-size: 14px; font-weight: bold; padding: 5px; border-radius: 5px; border: 1px solid #888; background-color: white; color: black; }"
        "QPushButton:hover { background-color: #e0e0e0; }"
        "QPushButton:disabled { color: #888; background-color: #ddd; border: 1px solid #ccc; }"
        "QPushButton#connect_butt { color: #4CAF50; }"
        "QPushButton#stop_bt { color: #d32f2f; }"
        "QPushButton#spid_bt { color: #4CAF50; }"
        );

    update_port();
    update_baundrate();
    ui->baud_cb->setCurrentText("115200");
    ui->dataBit_cb->setCurrentText("8");
    ui->stopBit_cb->setCurrentText("1");
    ui->parity_cb->setCurrentText("No Parity");

    ui->kp_tb->setDisabled(true);
    ui->ki_tb->setDisabled(true);
    ui->kd_tb->setDisabled(true);
    ui->sp_tb->setDisabled(true);
    ui->pwm_tb->setDisabled(true);
    ui->spid_bt->setDisabled(true);
    ui->stop_bt->setDisabled(true);
    ui->pwm_bt->setDisabled(true);
    ui->dir_bt->setDisabled(true);

    plotConfig();
    init = false;
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

void MainWindow::receive_data()
{
    while (serial.canReadLine()) {
        QByteArray data = serial.readLine();
        QString line = QString::fromUtf8(data).trimmed();

        ui->rec_pl_tb->moveCursor(QTextCursor::End);
        ui->rec_pl_tb->insertPlainText(line + "\n");
        ui->rec_pl_tb->moveCursor(QTextCursor::End);

        if (line.contains("E_LST"))
        {
            is_tx_error = true;

            if (!stop) {
                on_stop_bt_clicked();
                QMessageBox::critical(this, "TX ERROR",
                                      "CONNECTION LOST (PC -> STM32)\n"
                                      "Please check Tx wiring.");
            }
        }


        if (line.contains("S_ACK"))
        {
            watchdog_timer->start();

            is_rx_error = false;
            if (!line.contains("E_LST")) {
                is_tx_error = false;
            }
        }

        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2 && parts[0] == "M_PLT") {
            bool ok;
            float currentSpeed = parts[1].toFloat(&ok);
            if (ok) {
                double currentTimeSec = (double)tick_timer.elapsed() / 1000.0;
                float alpha = 0.2f;
                if (valueBuff.isEmpty()) lastFilteredValue = currentSpeed;
                else lastFilteredValue = alpha * currentSpeed + (1.0f - alpha) * lastFilteredValue;

                valueBuff.append(lastFilteredValue); refBuff.append(refValue);
                float currentError = refValue - lastFilteredValue;
                errorBuff.append(currentError); timeBuff.append(currentTimeSec);

                avalueBuff.append(currentSpeed); arefBuff.append(refValue);
                atimeBuff.append(currentTimeSec); aErrorBuff.append(currentError);

                if (valueBuff.size() > 100000) {
                    valueBuff.removeFirst(); refBuff.removeFirst();
                    errorBuff.removeFirst(); timeBuff.removeFirst();
                }
            }
        }
    }
}

void MainWindow::on_watchdog_timeout()
{
    if (!serial.isOpen()) return;

    is_rx_error = true;

    if (!stop) {
        on_stop_bt_clicked();
        QMessageBox::critical(this, "RX ERROR",
                              "CONNECTION LOST (PC -> STM32)\n"
                              "Please check Rx wiring or Power!");
    }
}

void MainWindow::update_gui_loop()
{
    if (timeBuff.isEmpty() || stop) return;

    ui->lcd_val->display(valueBuff.last());
    ui->lcd_err->display(errorBuff.last());

    ui->plot->graph(0)->setData(timeBuff, refBuff);
    ui->plot->graph(1)->setData(timeBuff, valueBuff);
    ui->plotError->graph(0)->setData(timeBuff, errorBuff);

    double currentTime = timeBuff.last();
    double startTime = (currentTime > 10.0) ? currentTime - 10.0 : 0.0;

    ui->plot->xAxis->setRange(startTime, currentTime);
    ui->plotError->xAxis->setRange(startTime, currentTime);

    double maxVal = 130.0;
    if (!valueBuff.isEmpty() && !refBuff.isEmpty()) {
        double currentMaxSpeed = *std::max_element(valueBuff.constBegin(), valueBuff.constEnd());
        double currentRef = refBuff.last();
        maxVal = std::max(currentMaxSpeed, currentRef);
    }
    if (maxVal < 50) maxVal = 50;
    ui->plot->yAxis->setRange(0.0, maxVal * 1.2);

    double maxErr = 5.0;
    if (!errorBuff.isEmpty()) {
        auto result = std::minmax_element(errorBuff.constBegin(), errorBuff.constEnd());
        double minE = std::abs(*result.first);
        double maxE = std::abs(*result.second);
        maxErr = std::max(minE, maxE);
    }
    if (maxErr < 5) maxErr = 5;
    ui->plotError->yAxis->setRange(-maxErr * 1.2, maxErr * 1.2);

    ui->plot->replot(QCustomPlot::rpQueuedReplot);
    ui->plotError->replot(QCustomPlot::rpQueuedReplot);
}

void MainWindow::setLedColor(QLabel *label, QString text, QString color)
{
    label->setAlignment(Qt::AlignCenter);
    label->setText(text);

    QString textColor = (color == "grey" || color == "#00FF00") ? "black" : "white";

    QString style = QString(
                        "border: 2px solid #555;"
                        "border-radius: 10px;"
                        "background-color: %1;"
                        "color: %2;"
                        "font-weight: bold;"
                        "font-size: 12px;"
                        ).arg(color).arg(textColor);

    label->setStyleSheet(style);
}

void MainWindow::blink_error_loop()
{
    blink_state = !blink_state;

    if (is_tx_error) {
        setLedColor(ui->lb_led_tx, "Tx", blink_state ? "red" : "grey");
    } else {
        setLedColor(ui->lb_led_tx, "Tx", "#00FF00");
    }

    if (is_rx_error) {
        setLedColor(ui->lb_led_rx, "Rx", blink_state ? "red" : "grey");
    } else {
        setLedColor(ui->lb_led_rx, "Rx", "#00FF00");
    }
}

void MainWindow::aliveChecking()
{
    if (!serial.isOpen()) return;
    QString msg = "C_ALV" + formatData("");
    serial.write(msg.toUtf8());
    ui->trans_pl_tb->moveCursor(QTextCursor::End);
    ui->trans_pl_tb->insertPlainText(msg + "\n");
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
    QPen penSP;
    penSP.setStyle(Qt::SolidLine);
    penSP.setWidthF(2);
    penSP.setColor(Qt::blue);

    QPen penCurrent;
    penCurrent.setStyle(Qt::SolidLine);
    penCurrent.setWidthF(1.5);
    penCurrent.setColor(Qt::red);

    QPen penError;
    penError.setStyle(Qt::SolidLine);
    penError.setWidthF(1.5);
    penError.setColor(Qt::red);

    ui->plot->addGraph();
    ui->plot->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(0)->setPen(penSP);
    ui->plot->graph(0)->setName("Set point (RPM)");

    ui->plot->addGraph();
    ui->plot->graph(1)->setLineStyle(QCPGraph::lsLine);
    ui->plot->graph(1)->setPen(penCurrent);
    ui->plot->graph(1)->setName("Current Value");

    ui->plot->xAxis->setLabel("Time (s)");
    ui->plot->yAxis->setLabel("Speed (rpm)");

    ui->plot->setNotAntialiasedElements(QCP::aeAll);
    QFont legendFont = font();
    legendFont.setPointSize(8);

    ui->plot->legend->setVisible(true);
    ui->plot->legend->setFont(legendFont);
    ui->plot->legend->setBrush(QBrush(QColor(255, 255, 255, 230)));
    ui->plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    ui->plotError->addGraph();
    ui->plotError->graph(0)->setPen(penError);
    ui->plotError->graph(0)->setName("Error");

    ui->plotError->xAxis->setLabel("Time (s)");
    ui->plotError->yAxis->setLabel("Error");

    ui->plotError->setNotAntialiasedElements(QCP::aeAll);

    ui->plotError->legend->setVisible(true);
    ui->plotError->legend->setFont(legendFont);
    ui->plotError->legend->setBrush(QBrush(QColor(255, 255, 255, 230)));
    ui->plotError->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    connect(ui->plot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plotError->xAxis, SLOT(setRange(QCPRange)));
    connect(ui->plotError->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->plot->xAxis, SLOT(setRange(QCPRange)));
}

void MainWindow::on_connect_butt_clicked()
{
    if (ui->connect_butt->text() == "CONNECT")
    {
        serial.setPortName(ui->port_cb->currentText());
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
            is_tx_error = false;
            is_rx_error = false;

            aliveChecking();

            ui->connect_butt->setText("DISCONNECT");
            ui->connect_butt->setStyleSheet(
                "font-size: 14px; font-weight: bold; padding: 5px; border-radius: 5px; border: 1px solid #888;"
                "color: red;"
                );

            ui->port_cb->setDisabled(true);
            ui->baud_cb->setDisabled(true);
            ui->dataBit_cb->setDisabled(true);
            ui->stopBit_cb->setDisabled(true);
            ui->parity_cb->setDisabled(true);

            ui->spid_bt->setDisabled(false);
            ui->stop_bt->setDisabled(false);
            ui->pwm_bt->setDisabled(false);
            ui->dir_bt->setDisabled(false);
            ui->kp_tb->setDisabled(false);
            ui->ki_tb->setDisabled(false);
            ui->kd_tb->setDisabled(false);
            ui->sp_tb->setDisabled(false);
            ui->pwm_tb->setDisabled(false);

            timer.stop();
            check_alive_timer.start();
            watchdog_timer->start();

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

            watchdog_timer->stop();
            check_alive_timer.stop();

            is_tx_error = false;
            is_rx_error = false;
            setLedColor(ui->lb_led_tx, "Tx", "grey");
            setLedColor(ui->lb_led_rx, "Rx", "grey");
        }
    }
    else
    {
        serial.close();

        ui->connect_butt->setText("CONNECT");
        ui->connect_butt->setStyleSheet(
            "font-size: 14px; font-weight: bold; padding: 5px; border-radius: 5px; border: 1px solid #888;"
            "color: #4CAF50;"
            );

        ui->spid_bt->setStyleSheet("");
        ui->stop_bt->setStyleSheet("");
        ui->dir_bt->setStyleSheet("");
        ui->pwm_bt->setStyleSheet("");

        ui->port_cb->setDisabled(false);
        ui->baud_cb->setDisabled(false);
        ui->dataBit_cb->setDisabled(false);
        ui->stopBit_cb->setDisabled(false);
        ui->parity_cb->setDisabled(false);

        ui->spid_bt->setDisabled(true);
        ui->stop_bt->setDisabled(true);
        ui->dir_bt->setDisabled(true);
        ui->kp_tb->setDisabled(true);
        ui->ki_tb->setDisabled(true);
        ui->kd_tb->setDisabled(true);
        ui->sp_tb->setDisabled(true);
        ui->pwm_tb->setDisabled(true);
        ui->pwm_bt->setDisabled(true);

        timer.start();
        watchdog_timer->stop();
        check_alive_timer.stop();

        is_tx_error = false;
        is_rx_error = false;
        setLedColor(ui->lb_led_tx, "Tx", "grey");
        setLedColor(ui->lb_led_rx, "Rx", "grey");

        ui->status_lb->setText("DISCONNECTED");
        ui->status_lb->setAlignment(Qt::AlignCenter);
        ui->status_lb->setStyleSheet("background-color: #cccccc; color: black; font-weight: bold; border-radius: 5px;");

        ui->statusbar->showMessage("Disconnected", 3000);
    }
}

void MainWindow::on_stop_bt_clicked()
{
    stop = true;
    QString msg = "M_STP" + formatData("");
    serial.write((msg ).toUtf8());

    ui->trans_pl_tb->append(msg);

    tick_timer.elapsed();
    preTime = 0;
}

void MainWindow::on_spid_bt_clicked()
{
    stop = false;
    refBuff.clear(); timeBuff.clear();
    valueBuff.clear(); errorBuff.clear();
    tick_timer.restart();

    QString kp = ui->kp_tb->text();
    QString ki = ui->ki_tb->text();
    QString kd = ui->kd_tb->text();
    QString sp = ui->sp_tb->text();

    if(kp.isEmpty()) kp = "0";
    if(ki.isEmpty()) ki = "0";
    if(kd.isEmpty()) kd = "0";
    if(sp.isEmpty()) sp = "0";

    refValue = sp.toFloat();
    QString payload = kp + " " + ki + " " + kd + " " + sp;
    QString msg = "M_STR" + formatData(payload);
    serial.write(msg.toUtf8());
    ui->trans_pl_tb->append(msg);
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
    if (freqText.isEmpty()) freqText = "0";

    QString msg = "M_FRE" + formatData(freqText);
    serial.write((msg).toUtf8());
    ui->trans_pl_tb->append(msg);
}

void MainWindow::on_dir_bt_clicked()
{
    QString msg = "M_INV" + formatData("");
    serial.write((msg ).toUtf8());
    ui->trans_pl_tb->append( msg);
}
