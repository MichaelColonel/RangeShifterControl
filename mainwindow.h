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

#pragma once

#include <QMainWindow>
#include <QSerialPort>

#include <bitset>

#include "defines.h"

namespace Ui {
class MainWindow;
}

class QSettings;
class QLabel;
class QAbstractButton;
class QTimer;
class QProgressDialog;
class QDateTime;

class OpcUaClient;
class OpcUaClientDialog;

typedef std::bitset<RS> RangeShifterBitSet;
typedef std::bitset<SWITCHES> SwitchesBitSet;

enum CommandType {
    NONE_COMMAND,
    TEST_COMMAND,
    STOP_COMMAND,
    START_COMMAND,
    STATE_COMMAND
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *);

public slots:
    void onOpcUaClientDialog();
    void onOpcUaClientConnected();
    void connectDeviceClicked();
    void disconnectDeviceClicked();
    void serialPortBytesWritten(qint64);
    void serialPortDataReady();
    void serialPortError(QSerialPort::SerialPortError);
    void rangeShifter1ButtonClicked(QAbstractButton*);
    void rangeShifter2ButtonClicked(QAbstractButton*);
    void rangeShifter3ButtonClicked(QAbstractButton*);
    void rangeShifter4ButtonClicked(QAbstractButton*);
    void rangeShifter5ButtonClicked(QAbstractButton*);
    void rangeShifter6ButtonClicked(QAbstractButton*);
    void rangeShiftersButtonClicked(QAbstractButton*);
    void rangeShiftersRowChanged(int);
    void stopButtonClicked();
    void startButtonClicked();
    void switchesStateButtonClicked();
    void testButtonClicked();
    void onOpcUaTimeout();

signals:
    void signalRangeShifterChanged( int, double, const QDateTime&);
    void signalStateChanged( int, const QDateTime&);

private:
    void saveSettings(QSettings* set);
    void loadSettings(QSettings* set);
    void showSwitchesState(const SwitchesBitSet&);
    void resetSwitchesState();
    void rangeShifterValueChanged(const RangeShifterBitSet&);
    void rangeShifterValues( int& code, double& thickness) const;

    Ui::MainWindow* ui;
    QSerialPort* port;
    QSettings* settings;
    QProgressDialog* progress_dialog;
    QLabel* switch_label[SWITCHES];
    QLabel* range_shifter_label[RS];
    QTimer* command_timer;
    QTimer* opcua_timer;
    QByteArray input_buffer;
    double* range_shifter_thickness;
    RangeShifterBitSet range_shifter_code;
    SwitchesBitSet switches_code;
    CommandType command_type;
    OpcUaClient* opcua_client;
    OpcUaClientDialog* opcua_dialog;
};
