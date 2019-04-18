/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

#include <QTextStream>
#include <QMessageBox>
#include <QSettings>
#include <QProgressDialog>
#include <QCloseEvent>
#include <QTimer>
#include <QDateTime>

#include <iomanip>
#include <iostream>
#include <cstring>

#include "opcuaclient.h"
#include "opcuaclientdialog.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"

namespace {

const char* const OK_string = "OK";
const char* const NO_string = "NO";
const char* const SwitchesState_response = "I!";
const char* const Stop_response = "E!";
const char* const Test_response = "T!";
const char* const Stop_string = "E\r";
const char* const SwitchesState_string = "I\r";
const char* const Test_string = "T\r";

const double range_shifter_thickness_default[RS] = { 1., 2., 4., 8., 16., 32. };

}

MainWindow::MainWindow(QWidget *parent)
    :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    port(0),
    settings(new QSettings( "BeamDegrader", "configure")),
    progress_dialog(new QProgressDialog( tr("Absorbers movement..."), \
        tr("Stop movement"), 0, 0, this)),
    command_timer(new QTimer(this)),
    opcua_timer(new QTimer(this)),
    range_shifter_thickness(new double[RS]),
    range_shifter_code(0),
    command_type(NONE_COMMAND),
    opcua_client(new OpcUaClient(this)),
    opcua_dialog(new OpcUaClientDialog( opcua_client, settings, false, this))
{
    ui->setupUi(this);

    switch_label[0] = ui->unset1Label;
    switch_label[1] = ui->set1Label;
    switch_label[2] = ui->unset2Label;
    switch_label[3] = ui->set2Label;
    switch_label[4] = ui->unset3Label;
    switch_label[5] = ui->set3Label;
    switch_label[6] = ui->unset4Label;
    switch_label[7] = ui->set4Label;
    switch_label[8] = ui->unset5Label;
    switch_label[9] = ui->set5Label;
    switch_label[10] = ui->unset6Label;
    switch_label[11] = ui->set6Label;

    range_shifter_label[0] = ui->rangeShifter1Label;
    range_shifter_label[1] = ui->rangeShifter2Label;
    range_shifter_label[2] = ui->rangeShifter3Label;
    range_shifter_label[3] = ui->rangeShifter4Label;
    range_shifter_label[4] = ui->rangeShifter5Label;
    range_shifter_label[5] = ui->rangeShifter6Label;

    const double* const rs = range_shifter_thickness_default;
    std::memcpy( range_shifter_thickness, rs, RS * sizeof(double));

    for ( int i = 0; i != RS; ++i) {
        range_shifter_label[i]->setText(tr("%1 mm, No %2").arg(range_shifter_thickness[RS - i - 1]).arg(i + 1));
    }

    for ( int i = 0; i != RS_VALUE; ++i) {
        double thick = 0.;
        RangeShifterBitSet value(i);
        for ( size_t j = 0; j != value.size(); ++j) {
            if (value[j]) {
                thick += range_shifter_thickness[j];
            }
        }
        QString text = tr("%1 mm").arg( thick, int(5), 'g', 2);
        QListWidgetItem* item = new QListWidgetItem( text, ui->rangeShifterThicknessListWidget);
        item->setTextAlignment(Qt::AlignRight);
    }

    loadSettings(settings);

    command_timer->setInterval(1000);
    opcua_timer->setInterval(2000);

    connect( ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect( ui->actionOpcUaClientDialog, SIGNAL(triggered()), this, SLOT(onOpcUaClientDialog()));

    connect( ui->connectButton, SIGNAL(clicked()), this, SLOT(connectDeviceClicked()));
    connect( ui->disconnectButton, SIGNAL(clicked()), this, SLOT(disconnectDeviceClicked()));
    connect( ui->rangeShifter1ButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(rangeShifter1ButtonClicked(QAbstractButton*)));
    connect( ui->rangeShifter2ButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(rangeShifter2ButtonClicked(QAbstractButton*)));
    connect( ui->rangeShifter3ButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(rangeShifter3ButtonClicked(QAbstractButton*)));
    connect( ui->rangeShifter4ButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(rangeShifter4ButtonClicked(QAbstractButton*)));
    connect( ui->rangeShifter5ButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(rangeShifter5ButtonClicked(QAbstractButton*)));
    connect( ui->rangeShifter6ButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(rangeShifter6ButtonClicked(QAbstractButton*)));
    connect( ui->rangeShiftersButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(rangeShiftersButtonClicked(QAbstractButton*)));
    connect( ui->rangeShifterThicknessListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(rangeShiftersRowChanged(int)));

    connect( ui->stopButton, SIGNAL(clicked()), this, SLOT(stopButtonClicked()));
    connect( ui->startButton, SIGNAL(clicked()), this, SLOT(startButtonClicked()));
    connect( ui->switchesStateButton, SIGNAL(clicked()), this, SLOT(switchesStateButtonClicked()));

    connect( opcua_client, SIGNAL(connected()), this, SLOT(onOpcUaClientConnected()));
    connect( opcua_client, SIGNAL(connected()), opcua_timer, SLOT(start()));
    connect( opcua_client, SIGNAL(disconnected()), opcua_timer, SLOT(stop()));
    connect( opcua_timer, SIGNAL(timeout()), this, SLOT(onOpcUaTimeout()));
    connect( command_timer, SIGNAL(timeout()), this, SLOT(switchesStateButtonClicked()));
    connect( progress_dialog, SIGNAL(canceled()), this, SLOT(stopButtonClicked()));

    connect( this, SIGNAL(signalRangeShifterChanged(int,double,QDateTime)), opcua_dialog, SLOT(setRangeShifterValue(int,double,QDateTime)));
    connect( this, SIGNAL(signalRangeShifterChanged(int,double,QDateTime)), opcua_client, SLOT(writeRangeShifterValue(int,double,QDateTime)));
    connect( this, SIGNAL(signalStateChanged(int,QDateTime)), opcua_client, SLOT(writeStateValue(int,QDateTime)));
    connect( this, SIGNAL(signalStateChanged(int,QDateTime)), opcua_dialog, SLOT(setStateValue(int,QDateTime)));

    rangeShifterValueChanged(range_shifter_code);

    progress_dialog->setWindowModality(Qt::WindowModal);
    progress_dialog->setWindowTitle("Moving Progress");

    bool opcua_at_startup = settings->value( "opcua-connect-at-startup", false).toBool();
    if (opcua_at_startup) {
        QTimer::singleShot( 1000, opcua_dialog, SLOT(onStartUpConnection()));
    }
}

MainWindow::~MainWindow()
{
    delete opcua_dialog;
    delete settings;
    delete ui;
    delete [] range_shifter_thickness;
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
    int res = QMessageBox::Yes;
    if (port && port->isOpen()) {
        res = QMessageBox::warning( this, tr("Close program"), \
            tr("Serial device is still connected.\nDo you want to quit program?"), \
            QMessageBox::Yes, QMessageBox::No | QMessageBox::Default);
    }

    if (res == QMessageBox::Yes) {
        disconnectDeviceClicked();
        if (opcua_client && opcua_client->isConnected()) {
            int value = 0;
            QDateTime now = QDateTime::currentDateTime();
            bool res = opcua_client->writeHeartBeatValue( value, now);
            std::cout << "OPC UA HeartBeat result: " << res << ", value: " << value << std::endl;
        }
        saveSettings(settings);
        event->accept();
    }
    else {
        event->ignore();
    }
}

void
MainWindow::saveSettings(QSettings* set)
{
    QString name = ui->deviceNameLineEdit->text();
    set->setValue( "device-name", name);

    int value = ui->rangeShifterThicknessListWidget->currentRow();
    set->setValue( "range-shifter-index", value);
}

void
MainWindow::loadSettings(QSettings* set)
{
    QString name = set->value( "device-name", "/dev/tty1").toString();
    ui->deviceNameLineEdit->setText(name);

    int value = set->value( "range-shifter-index", 0).toInt();
    ui->rangeShifterThicknessListWidget->setCurrentRow(value);
    range_shifter_code = RangeShifterBitSet(value);
}

void
MainWindow::connectDeviceClicked()
{
    QString name = ui->deviceNameLineEdit->text();

    port = new QSerialPort( name, this);

    if (port && port->open(QIODevice::ReadWrite)) {
        port->setBaudRate(QSerialPort::Baud2400);
        port->setDataBits(QSerialPort::Data8);
        port->setParity(QSerialPort::NoParity);
        port->setStopBits(QSerialPort::OneStop);
        port->setFlowControl(QSerialPort::NoFlowControl);

        port->flush();
        connect( port, SIGNAL(readyRead()), this, SLOT(serialPortDataReady()));
        connect( port, SIGNAL(bytesWritten(qint64)), this, SLOT(serialPortBytesWritten(qint64)));
        connect( port, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(serialPortError(QSerialPort::SerialPortError)));

        ui->connectButton->setEnabled(false);
        ui->disconnectButton->setEnabled(true);
        ui->startButton->setEnabled(true);
        ui->stopButton->setEnabled(true);
        ui->switchesStateButton->setEnabled(true);

        ui->statusBar->showMessage( tr("Device connected"), 1000);
        QTimer::singleShot( 1000, this, SLOT(testButtonClicked()));

        emit signalStateChanged( 1, QDateTime::currentDateTime());
    }
    else if (port && !port->isOpen()) {
        QMessageBox msgBox(this);
        msgBox.setModal(true);
        msgBox.setWindowTitle(tr("Range shifter control"));
        msgBox.setText(tr("Unable to connect device %1.\n%2.").arg(port->portName()).arg(port->errorString()));
        msgBox.exec();
        ui->statusBar->showMessage( tr("Unable to connect device"), 1000);

        delete port;
        port = 0;
        if (opcua_client && opcua_client->isConnected())
            emit signalStateChanged( 0, QDateTime::currentDateTime());
    }
}

void
MainWindow::disconnectDeviceClicked()
{
    command_timer->stop();
    if (port && port->isOpen()) {
        port->close();
        delete port;
        port = 0;
    }
    else if (port && !port->isOpen()) {
        delete port;
        port = 0;
    }

    emit signalStateChanged( 0, QDateTime::currentDateTime());

    resetSwitchesState();
    ui->connectButton->setEnabled(true);
    ui->disconnectButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    ui->switchesStateButton->setEnabled(false);
    ui->statusBar->showMessage( tr("Device disconnected"), 1000);
}

void
MainWindow::serialPortDataReady()
{
    QTextStream output(stdout);
    QByteArray bytes_array = port->readAll();
    output << bytes_array;

    for ( QByteArray::ConstIterator it = bytes_array.begin(); it != bytes_array.end(); ++it) {
        if (*it == '\n' || *it == '\r') {
            if (input_buffer.endsWith(OK_string) && input_buffer.contains(SwitchesState_response)) {
                int pos = input_buffer.lastIndexOf(SwitchesState_response);
                int end = input_buffer.lastIndexOf(OK_string);
                pos += QByteArray(SwitchesState_response).size();
                QByteArray num = input_buffer.mid( pos, end - pos);
                ushort v = num.toUShort();
                switches_code = SwitchesBitSet(v);
                showSwitchesState(switches_code);
                QString code_string = QString::fromStdString(switches_code.to_string());
                if (command_type == STATE_COMMAND) {
                    output << "Switches command OK! Value: " << v << ", Code: " << code_string << endl;
                    ui->statusBar->showMessage( tr("Switches state OK!"), 1000);
                    command_type = NONE_COMMAND;
                } else if (command_type == NONE_COMMAND) {
                    output << "Internal controller switches command OK! Value: " << v;
                    output << ", Code: " << code_string << endl;
                }
            }
            else if (input_buffer.endsWith("!OK") && input_buffer.contains("D")) {
                int pos = input_buffer.lastIndexOf("D");
                int end = input_buffer.lastIndexOf("!OK");
                pos++;
                QByteArray num = input_buffer.mid( pos, end - pos);
                ushort v = num.toUShort();
                RangeShifterBitSet value(v);
                QString code_string = QString::fromStdString(value.to_string());
                if (command_type == START_COMMAND) {
                    output << "Start command OK! Value: " << v << ", Code: " << code_string << endl;
                    ui->statusBar->showMessage( tr("Start command OK!"), 1000);
                    command_type = NONE_COMMAND;
                    command_timer->setInterval(4000);
                    command_timer->start();
                }
            }
            else if (input_buffer.endsWith(OK_string) && input_buffer.contains(Stop_response)) {
                if (command_type == STOP_COMMAND) {
                    output << "Stop command OK! Check switches status." << endl;
                    ui->statusBar->showMessage( tr("Stop command OK!"), 1000);
                    command_type = NONE_COMMAND;
                }
                else if (command_type == NONE_COMMAND) {
                    int code;
                    double thickness;
                    QDateTime dt = QDateTime::currentDateTime();
                    rangeShifterValues( code, thickness);
                    emit signalRangeShifterChanged( code, thickness, dt);
                    output << "Internal controller switches stop command OK!" << endl;
                }
                command_timer->setInterval(1000);
                command_timer->start();
            }
            else if (input_buffer.endsWith(OK_string) && input_buffer.contains(Test_response)) {
                if (command_type == TEST_COMMAND) {
                    output << "Test command OK!" << endl;
                    ui->statusBar->showMessage( tr("Test command OK!"), 1000);
                    command_type = NONE_COMMAND;
                    command_timer->start();
                }
            }
            else if (input_buffer.endsWith(NO_string)) {
                output << endl << "Command answer contains \"NO\", something went wrong!" << endl;
            }
            else {
            }
            input_buffer.clear();
        }
        else {
            input_buffer.append(*it);
        }
    }
}

void
MainWindow::serialPortError(QSerialPort::SerialPortError error)
{
    QTextStream output(stderr);

    if (error == QSerialPort::ReadError) {
        output << QObject::tr("An I/O error occurred while reading the data from port %1, error: %2").arg(port->portName()).arg(port->errorString()) << endl;
    }
}

void
MainWindow::startButtonClicked()
{
    int code;
    double dummy;
    rangeShifterValues( code, dummy);

    QString text = QString("D%1\r").arg(code);
    QByteArray start_command;
    start_command.append(text);

    std::cout << "Command Code Value: " << std::setw(2) << code << std::endl;

    if (port && port->isOpen()) {
        command_type = START_COMMAND;
        command_timer->stop();
        port->write(start_command);
    }
}

void
MainWindow::switchesStateButtonClicked()
{
    if (port && port->isOpen()) {
        command_type = STATE_COMMAND;
        port->write(SwitchesState_string);
    }
}

void
MainWindow::stopButtonClicked()
{
    if (port && port->isOpen()) {
        command_type = STOP_COMMAND;
        port->write(Stop_string);
    }
}

void
MainWindow::testButtonClicked()
{
    if (port && port->isOpen()) {
        command_type = TEST_COMMAND;
        port->write(Test_string);
    }
}

void
MainWindow::showSwitchesState(const SwitchesBitSet& value)
{
    for ( size_t i = 0; i != value.size(); ++i) {
        if (value[i])
            switch_label[i]->setPixmap(QPixmap(":/indicators/icons/green.png"));
        else
            switch_label[i]->setPixmap(QPixmap(":/indicators/icons/red.png"));
    }
}

void
MainWindow::resetSwitchesState()
{
    for ( int i = 0; i != SWITCHES; ++i)
        switch_label[i]->setPixmap(QPixmap(":/indicators/icons/gray.png"));
}

void
MainWindow::rangeShiftersRowChanged(int value)
{
    int code;
    double thickness;
    RangeShifterBitSet& rs_code = range_shifter_code;
    rs_code = RangeShifterBitSet(value);
    rangeShifterValueChanged(rs_code);
    rangeShifterValues( code, thickness);
    std::cout << "New Value: " << std::setw(2) << value << ", ";
    std::cout << "Bits: " << std::setw(rs_code.size()) << rs_code << std::endl;
    std::cout << "Code: "  << std::setw(2) << code << ", thickness: ";
    std::cout << std::setprecision(2) << thickness << " mm" << std::endl;
}

void
MainWindow::rangeShifterValueChanged(const RangeShifterBitSet& value)
{
    if (value.test(5))
        ui->set1RadioButton->setChecked(true);
    else
        ui->unset1RadioButton->setChecked(true);

    if (value.test(4))
        ui->set2RadioButton->setChecked(true);
    else
        ui->unset2RadioButton->setChecked(true);

    if (value.test(3))
        ui->set3RadioButton->setChecked(true);
    else
        ui->unset3RadioButton->setChecked(true);

    if (value.test(2))
        ui->set4RadioButton->setChecked(true);
    else
        ui->unset4RadioButton->setChecked(true);

    if (value.test(1))
        ui->set5RadioButton->setChecked(true);
    else
        ui->unset5RadioButton->setChecked(true);

    if (value.test(0))
        ui->set6RadioButton->setChecked(true);
    else
        ui->unset6RadioButton->setChecked(true);
}

void
MainWindow::rangeShifter1ButtonClicked(QAbstractButton* button)
{
    RangeShifterBitSet& rs_code = range_shifter_code;

    QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);
    if (rbutton == ui->set1RadioButton)
        rs_code.set(5);
    else if (rbutton == ui->unset1RadioButton)
        rs_code.reset(5);

    int v = static_cast<int>(rs_code.to_ulong());
    ui->rangeShifterThicknessListWidget->setCurrentRow(v);
}

void
MainWindow::rangeShifter2ButtonClicked(QAbstractButton* button)
{
    RangeShifterBitSet& rs_code = range_shifter_code;

    QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);
    if (rbutton == ui->set2RadioButton)
        rs_code.set(4);
    else if (rbutton == ui->unset2RadioButton)
        rs_code.reset(4);

    int v = static_cast<int>(rs_code.to_ulong());
    ui->rangeShifterThicknessListWidget->setCurrentRow(v);
}

void
MainWindow::rangeShifter3ButtonClicked(QAbstractButton* button)
{
    RangeShifterBitSet& rs_code = range_shifter_code;

    QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);
    if (rbutton == ui->set3RadioButton)
        rs_code.set(3);
    else if (rbutton == ui->unset3RadioButton)
        rs_code.reset(3);

    int v = static_cast<int>(rs_code.to_ulong());
    ui->rangeShifterThicknessListWidget->setCurrentRow(v);
}

void
MainWindow::rangeShifter4ButtonClicked(QAbstractButton* button)
{
    RangeShifterBitSet& rs_code = range_shifter_code;

    QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);
    if (rbutton == ui->set4RadioButton)
        rs_code.set(2);
    else if (rbutton == ui->unset4RadioButton)
        rs_code.reset(2);

    int v = static_cast<int>(rs_code.to_ulong());
    ui->rangeShifterThicknessListWidget->setCurrentRow(v);
}

void
MainWindow::rangeShifter5ButtonClicked(QAbstractButton* button)
{
    RangeShifterBitSet& rs_code = range_shifter_code;

    QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);
    if (rbutton == ui->set5RadioButton)
        rs_code.set(1);
    else if (rbutton == ui->unset5RadioButton)
        rs_code.reset(1);

    int v = static_cast<int>(rs_code.to_ulong());
    ui->rangeShifterThicknessListWidget->setCurrentRow(v);
}

void
MainWindow::rangeShifter6ButtonClicked(QAbstractButton* button)
{
    RangeShifterBitSet& rs_code = range_shifter_code;

    QRadioButton* rbutton = qobject_cast<QRadioButton*>(button);
    if (rbutton == ui->set6RadioButton)
        rs_code.set(0);
    else if (rbutton == ui->unset6RadioButton)
        rs_code.reset(0);

    int v = static_cast<int>(rs_code.to_ulong());
    ui->rangeShifterThicknessListWidget->setCurrentRow(v);
}

void
MainWindow::rangeShiftersButtonClicked(QAbstractButton* button)
{
    RangeShifterBitSet& rs_code = range_shifter_code;

    QPushButton* pbutton = qobject_cast<QPushButton*>(button);
    if (pbutton && pbutton == ui->setAllButton)
        rs_code.set();
    else if (pbutton && pbutton == ui->unsetAllButton)
        rs_code.reset();

    int v = static_cast<int>(rs_code.to_ulong());
    ui->rangeShifterThicknessListWidget->setCurrentRow(v);
}

void
MainWindow::serialPortBytesWritten(qint64 bytes)
{
    std::cout << "Bytes written: " << bytes << std::endl;
}

void
MainWindow::onOpcUaClientDialog()
{
    opcua_dialog->show();
}

void
MainWindow::onOpcUaClientConnected()
{
    ui->statusBar->showMessage( "OPC UA server connection successfull", 1000);
}

void
MainWindow::onOpcUaTimeout()
{
    if (opcua_client && opcua_client->isConnected()) {
        int value = 1;
        QDateTime now = QDateTime::currentDateTime();
        bool res = opcua_client->writeHeartBeatValue( value, now);
        if (opcua_dialog) opcua_dialog->setHeatBeatValue( value, now);
        std::cout << "OPC UA HeartBeat result: " << res << ", value: " << value << std::endl;
    }
}

void
MainWindow::rangeShifterValues( int& code, double& thickness) const
{
    RangeShifterBitSet tmp;
    const RangeShifterBitSet& rs_code = range_shifter_code;
    thickness = 0.;

    for ( size_t i = 0; i != rs_code.size(); ++i) {
        if (rs_code[i]) {
            tmp.set(rs_code.size() - i - 1);
            thickness += range_shifter_thickness[i];
        }
    }
    code = static_cast<int>(tmp.to_ulong());
}
