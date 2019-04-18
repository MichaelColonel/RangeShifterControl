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
 */

#pragma once

#include <array>
#include <map>
#include <QObject>
#include <QString>

#include "open62541.h"

class QDateTime;

enum ChildrenNodesType {
    STATE_NODE,
    HEART_BEAT_NODE,
    VALUE_CODE_NODE,
    VALUE_THICKNESS_NODE
};

class OpcUaClient : public QObject {
    Q_OBJECT
public:

    OpcUaClient(QObject* parent = 0);
    virtual ~OpcUaClient();
    UA_StatusCode connect( const QString& server, int port,
        const UA_ClientConfig& config = UA_ClientConfig_default);
    UA_StatusCode connect( const QString& path,
        const UA_ClientConfig& config = UA_ClientConfig_default);
    void disconnect();
    bool isConnected() const;
    static UA_StatusCode nodeIterCallback( UA_NodeId childId,
        UA_Boolean isInverse, UA_NodeId referenceTypeId,
        void* handle);
    void setChildNode( const UA_NodeId& child, ChildrenNodesType type);

public slots:
    void iterate();
    bool writeHeartBeatValue( int, const QDateTime& dt);
    bool writeStateValue( int, const QDateTime& dt);
    bool writeRangeShifterValue( int, double, const QDateTime& dt);

signals:
    void disconnected();
    void connected();

private:
    UA_Client* client;
    QString server_path;
    int server_port;
    std::map< ChildrenNodesType, UA_NodeId > children_nodes;
};
