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

#include <QDateTime>
#include <iostream>

#include "opcuaclient.h"

namespace {

const UA_String parent_node_str = UA_STRING_STATIC("RBS.BeamRangeShifter.01");
const UA_String heart_beat_str = UA_STRING_STATIC("HeartBeat");
const UA_String state_str = UA_STRING_STATIC("State");
const UA_String value_code_str =  UA_STRING_STATIC("ValueCode");
const UA_String value_thickness_str =  UA_STRING_STATIC("ValueThickness");

}

OpcUaClient::OpcUaClient(QObject* parent)
    :
    QObject(parent),
    client(nullptr),
    server_path("opc.tcp://localhost"),
    server_port(4840)
{
}

void
OpcUaClient::disconnect()
{
    if (client) {
        UA_Client_disconnect(client);
        emit disconnected();
        UA_Client_delete(client); /* Disconnects the client internally */
        client = nullptr;
    }
}

OpcUaClient::~OpcUaClient()
{
    disconnect();
}

UA_StatusCode
OpcUaClient::connect( const QString& path, int port, const UA_ClientConfig& config)
{
    server_path = path;
    server_port = port;

    QString server_string = QString("%1:%2").arg(server_path).arg(server_port);
    UA_StatusCode retval = connect( server_string, config);
    return retval;
}

UA_StatusCode
OpcUaClient::connect( const QString& path, const UA_ClientConfig& config)
{
    std::string serv_string = path.toStdString();

    client = UA_Client_new(config);

    UA_StatusCode retval = UA_Client_connect( client, serv_string.c_str());
    if (retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        client = nullptr;
    }
    else {

        char* str = reinterpret_cast<char*>(parent_node_str.data);
        UA_NodeId parent = UA_NODEID_STRING( 0, str);
        UA_NodeId* res = UA_NodeId_new();
        retval = UA_Client_readNodeIdAttribute( client, parent, res);
        UA_NodeId_delete(res);
        if (retval != UA_STATUSCODE_GOOD) {
            emit disconnected();
            disconnect();
        }
        else {

            retval = UA_Client_forEachChildNodeCall( client, parent, nodeIterCallback, this);

            if (retval == UA_STATUSCODE_GOOD) {
                emit connected();
            }
            else {
                emit disconnected();
                disconnect();
            }
        }
    }
    return retval;
}

bool
OpcUaClient::writeHeartBeatValue( int value,  const QDateTime&)
{
    if (!isConnected())
        return false;

    UA_Variant* variant = UA_Variant_new();
    UA_Variant_setScalarCopy( variant, &value, &UA_TYPES[UA_TYPES_INT16]);
    UA_NodeId& node = children_nodes[HEART_BEAT_NODE];
    UA_StatusCode rescode = UA_Client_writeValueAttribute( client, node, variant);
    UA_Variant_delete(variant);
    return bool(rescode == UA_STATUSCODE_GOOD);
}

bool
OpcUaClient::writeStateValue( int state,  const QDateTime&)
{
    if (!isConnected())
        return false;

    UA_Variant* variant = UA_Variant_new();
    UA_Variant_setScalarCopy( variant, &state, &UA_TYPES[UA_TYPES_INT16]);
    UA_NodeId& node = children_nodes[STATE_NODE];
    UA_StatusCode rescode = UA_Client_writeValueAttribute( client, node, variant);
    UA_Variant_delete(variant);
    return bool(rescode == UA_STATUSCODE_GOOD);
}

bool
OpcUaClient::writeRangeShifterValue( int code, double thickness, const QDateTime& datetime)
{
    if (!isConnected())
        return false;

    bool result = false;

    uint t = datetime.toTime_t();
    UA_DateTime dt = UA_DateTime_fromUnixTime(static_cast<UA_Int64>(t));

    void* ptr = UA_Array_new( 2, &UA_TYPES[UA_TYPES_WRITEVALUE]);
    UA_WriteValue* values = reinterpret_cast<UA_WriteValue*>(ptr);

    UA_Int16 value = code;
    UA_Float value2 = thickness;

    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = values;
    wReq.nodesToWriteSize = 2;
    UA_NodeId_copy( &children_nodes[VALUE_CODE_NODE], &wReq.nodesToWrite[0].nodeId);
//    wReq.nodesToWrite[0].nodeId = UA_NODEID_STRING_ALLOC( 0, "ValueCode");
    wReq.nodesToWrite[0].attributeId = UA_ATTRIBUTEID_VALUE;
    wReq.nodesToWrite[0].value.hasValue = true;
    wReq.nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_INT16];
    wReq.nodesToWrite[0].value.sourceTimestamp = dt;
    wReq.nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA_NODELETE; /* do not free the integer on deletion */
    wReq.nodesToWrite[0].value.value.data = &value;

    UA_NodeId_copy( &children_nodes[VALUE_THICKNESS_NODE], &wReq.nodesToWrite[1].nodeId);
//    wReq.nodesToWrite[1].nodeId = UA_NODEID_STRING_ALLOC( 0, "ValueThickness");
    wReq.nodesToWrite[1].attributeId = UA_ATTRIBUTEID_VALUE;
    wReq.nodesToWrite[1].value.hasValue = true;
    wReq.nodesToWrite[1].value.value.type = &UA_TYPES[UA_TYPES_FLOAT];
    wReq.nodesToWrite[1].value.sourceTimestamp = dt;
    wReq.nodesToWrite[1].value.value.storageType = UA_VARIANT_DATA_NODELETE; /* do not free the integer on deletion */
    wReq.nodesToWrite[1].value.value.data = &value2;

    UA_WriteResponse wResp = UA_Client_Service_write( client, wReq);
    if(wResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        result = true;
    }
    UA_WriteRequest_deleteMembers(&wReq);
    UA_WriteResponse_deleteMembers(&wResp);
    return result;
}

void
OpcUaClient::iterate()
{
    if (client)
        UA_Client_run_iterate( client, 0);
}

bool
OpcUaClient::isConnected() const
{
    if (client) {
        UA_ClientState res = UA_Client_getState(client);
        return (res == UA_CLIENTSTATE_CONNECTED ||
                res == UA_CLIENTSTATE_SECURECHANNEL ||
                res == UA_CLIENTSTATE_SESSION);
    }
    else
        return false;
}

UA_StatusCode
OpcUaClient::nodeIterCallback( UA_NodeId childId, UA_Boolean isInverse, UA_NodeId /* referenceTypeId */,
    void* handle)
{
    if(isInverse)
        return UA_STATUSCODE_GOOD;

    OpcUaClient* ptr = reinterpret_cast<OpcUaClient*>(handle);
/*
    printf( "%s ---> NodeId %d, %s\n",
        referenceTypeId.identifier.string.data, childId.namespaceIndex,
        childId.identifier.string.data);
*/
    if (ptr) {

        if (UA_String_equal( &childId.identifier.string, &heart_beat_str)) {
            std::cout << "Heart Beat Node OK!" << std::endl;
            ptr->setChildNode( childId, HEART_BEAT_NODE);
        }
        else if (UA_String_equal( &childId.identifier.string, &state_str)) {
            std::cout << "State Node OK!" << std::endl;
            ptr->setChildNode( childId, STATE_NODE);
        }
        else if (UA_String_equal( &childId.identifier.string, &value_code_str)) {
            std::cout << "Value Code Node OK!" << std::endl;
            ptr->setChildNode( childId, VALUE_CODE_NODE);
        }
        else if (UA_String_equal( &childId.identifier.string, &value_thickness_str)) {
            std::cout << "Value Thickness Node OK!" << std::endl;
            ptr->setChildNode( childId, VALUE_THICKNESS_NODE);
        }
    }
    return UA_STATUSCODE_GOOD;
}

void
OpcUaClient::setChildNode(const UA_NodeId &child, ChildrenNodesType type)
{
    UA_NodeId_copy( &child, &children_nodes[type]);
}
