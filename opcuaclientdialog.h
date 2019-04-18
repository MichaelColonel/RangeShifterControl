/*
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#pragma once

#include <QDialog>


namespace Ui {
class OpcUaClientDialog;
}


class QSettings;
class QTreeWidgetItem;
class QDateTime;

class OpcUaClient;

class OpcUaClientDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpcUaClientDialog( OpcUaClient* client,
        QSettings* settings,
        bool connect_on_start = false,
        QWidget *parent = 0);
    ~OpcUaClientDialog();

public slots:
    void onStartUpConnection();
    void onClientConnected();
    void setRangeShifterState( int, const QDateTime&);
    void setStateValue( int, const QDateTime&);
    void setHeatBeatValue( int, const QDateTime&);
    void setRangeShifterValue( int, double, const QDateTime&);

private slots:
    void onConnectClicked();
    void onDisconnectClicked();

private:
    Ui::OpcUaClientDialog *ui;
    OpcUaClient* opcua_client;
    QSettings* settings;
    QTreeWidgetItem* item_state;
    QTreeWidgetItem* item_code;
    QTreeWidgetItem* item_thickness;
    QTreeWidgetItem* item_heartbeat;
};
