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

#include <QTimer>
#include <QSettings>
#include <QDateTime>

#include <iostream>

#include "opcuaclient.h"

#include "opcuaclientdialog.h"
#include "ui_opcuaclientdialog.h"

OpcUaClientDialog::OpcUaClientDialog( OpcUaClient* client,
    QSettings* set, bool connect_at_start,
    QWidget *parent)
    :
    QDialog(parent),
    ui(new Ui::OpcUaClientDialog),
    opcua_client(client),
    settings(set),
    item_state(nullptr),
    item_code(nullptr),
    item_thickness(nullptr),
    item_heartbeat(nullptr)
{
    ui->setupUi(this);

    ui->opcUaNodesTreeWidget->expandAll();

    QString opcua_path = settings->value( "opcua-server-path", "opc.tcp://localhost").toString();
    ui->opcUaServerPathLineEdit->setText(opcua_path);

    int opcua_port = settings->value( "opcua-server-port", 4840).toInt();
    ui->opcUaServerPortSpinBox->setValue(opcua_port);

    ui->opcUaConnectAtStartupCheckBox->setChecked(connect_at_start);

    QPushButton* close_button = ui->buttonBox->button(QDialogButtonBox::Close);
    if (close_button)
        connect( close_button, SIGNAL(clicked()), this, SLOT(close()));

    connect( ui->connectPushButton, SIGNAL(clicked()), this, SLOT(onConnectClicked()));
    connect( ui->disconnectPushButton, SIGNAL(clicked()), this, SLOT(onDisconnectClicked()));

    if (opcua_client) {
        connect( opcua_client, SIGNAL(connected()), this, SLOT(onClientConnected()));
    }
    if (opcua_client && opcua_client->isConnected()) {
        ui->connectPushButton->setEnabled(false);
        ui->disconnectPushButton->setEnabled(true);
    }
    else {
        ui->connectPushButton->setEnabled(true);
        ui->disconnectPushButton->setEnabled(false);
    }

    QTreeWidgetItem* item = ui->opcUaNodesTreeWidget->topLevelItem(0);
    if (item) {
        item->setTextAlignment( 1, Qt::AlignHCenter | Qt::AlignVCenter);
        item->setTextAlignment( 2, Qt::AlignHCenter | Qt::AlignVCenter);
        item->setText( 1, tr("Disconnected"));

        QTreeWidgetItem* child = item->child(0);
        if (child) {
            child->setTextAlignment( 1, Qt::AlignHCenter | Qt::AlignVCenter);
            child->setTextAlignment( 2, Qt::AlignHCenter | Qt::AlignVCenter);
            item_state = child;           
        }
        child = item->child(1);
        if (child) {
            child->setTextAlignment( 1, Qt::AlignHCenter | Qt::AlignVCenter);
            child->setTextAlignment( 2, Qt::AlignHCenter | Qt::AlignVCenter);
            item_code = child;
        }
        child = item->child(2);
        if (child) {
            child->setTextAlignment( 1, Qt::AlignHCenter | Qt::AlignVCenter);
            child->setTextAlignment( 2, Qt::AlignHCenter | Qt::AlignVCenter);
            item_thickness = child;
        }
        child = item->child(3);
        if (child) {
            child->setTextAlignment( 1, Qt::AlignHCenter | Qt::AlignVCenter);
            child->setTextAlignment( 2, Qt::AlignHCenter | Qt::AlignVCenter);
            item_heartbeat = child;
        }
    }

    if (connect_at_start) {
        QTimer::singleShot( 1000, this, SLOT(onConnectClicked()));
    }
}

OpcUaClientDialog::~OpcUaClientDialog()
{
    bool state = ui->opcUaConnectAtStartupCheckBox->isChecked();
    settings->setValue( "opcua-connect-at-startup", state);

    QString server_path = ui->opcUaServerPathLineEdit->text();
    settings->setValue( "opcua-server-path", server_path);

    int server_port = ui->opcUaServerPortSpinBox->value();
    settings->setValue( "opcua-server-port", server_port);

    delete ui;
}

void
OpcUaClientDialog::setStateValue( int value, const QDateTime& datetime)
{
    if (item_state) {
        item_state->setText( 1, QString("%1").arg(value));
        item_state->setText( 2, datetime.toString(Qt::ISODate));
    }
}

void
OpcUaClientDialog::setHeatBeatValue( int value, const QDateTime& datetime)
{
    if (item_heartbeat) {
        item_heartbeat->setText( 1, QString("%1").arg(value));
        item_heartbeat->setText( 2, datetime.toString(Qt::ISODate));
    }
}

void
OpcUaClientDialog::setRangeShifterValue( int code, double thickness, const QDateTime& dt)
{
    QString dt_str = dt.toString(Qt::ISODate);
    if (item_code) {
        item_code->setText( 1, QString("%1").arg(code));
        item_code->setText( 2, dt_str);
    }
    if (item_thickness) {
        item_thickness->setText( 1, tr("%1 mm").arg( thickness, 10, 'g', 1));
        item_thickness->setText( 2, dt_str);
    }
}

void
OpcUaClientDialog::setRangeShifterState( int state, const QDateTime&)
{
    QTreeWidgetItem* item = ui->opcUaNodesTreeWidget->topLevelItem(0);
    if (item && state) {
        item->setText( 1, tr("Connected"));
    }
    else if (item && !state) {
        item->setText( 1, tr("Disconnected"));
    }
}

void
OpcUaClientDialog::onClientConnected()
{
    QTreeWidgetItem* item = ui->opcUaNodesTreeWidget->topLevelItem(0);
    if (item) {
        item->setText( 1, tr("Connected"));
    }

    ui->connectPushButton->setEnabled(false);
    ui->disconnectPushButton->setEnabled(true);
}

void
OpcUaClientDialog::onConnectClicked()
{
    if (opcua_client && !opcua_client->isConnected()) {

        QString opcua_path = ui->opcUaServerPathLineEdit->text();
        int opcua_port = ui->opcUaServerPortSpinBox->value();

        QString path = QString("%1:%2").arg(opcua_path).arg(opcua_port);

        UA_StatusCode code = opcua_client->connect(path);
        if (code == UA_STATUSCODE_GOOD) {
            ui->connectPushButton->setEnabled(false);
            ui->disconnectPushButton->setEnabled(true);
        }
        else {
            ui->connectPushButton->setEnabled(true);
            ui->disconnectPushButton->setEnabled(false);
        }
    }
}

void
OpcUaClientDialog::onDisconnectClicked()
{
    if (opcua_client && opcua_client->isConnected()) {
        QDateTime now = QDateTime::currentDateTime();
        if (opcua_client->writeHeartBeatValue( 0, now)) {
            setHeatBeatValue( 0, now);
        }

        opcua_client->disconnect();
        QTreeWidgetItem* item = ui->opcUaNodesTreeWidget->topLevelItem(0);
        if (item) {
            item->setText( 1, tr("Disconnected"));
        }
        ui->connectPushButton->setEnabled(true);
        ui->disconnectPushButton->setEnabled(false);
    }
}

void
OpcUaClientDialog::onStartUpConnection()
{
    ui->opcUaConnectAtStartupCheckBox->setChecked(true);
    onConnectClicked();
    show();
}
