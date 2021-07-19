#include "p2pServer.h"
#include "codec.h"

#include "muduo/base/Logging.h"
#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"

#include <map>
#include <list>

using namespace muduo;
using namespace muduo::net;

typename p2pServer::ConnInfo* p2pServer::getConnInfo(const TcpConnectionPtr& conn)
{
    assert(!conn->getContext().empty());
    return boost::any_cast<ConnInfo>(conn->getMutableContext());

}

p2pServer::p2pServer(EventLoop* loop, const InetAddress& listenAddr, int idleSeconds)
        : _server(loop, listenAddr, "p2pServer"),
          _codec(std::bind(&p2pServer::onMessage, this, _1, _2, _3)),
          _idleSeconds(idleSeconds)
{
    _server.setConnectionCallback(std::bind(&p2pServer::onConnection, this, _1));
    _server.setMessageCallback(std::bind(&LengthHeaderCodec::onMessage, &_codec, _1, _2, _3));
    loop->runEvery(1.0, std::bind(&p2pServer::onTimer, this)); // every one second check the timeout connection
}

void p2pServer::start()
{
    _server.setThreadNum(1);
    _server.start();
}

void p2pServer::onTimer()
{
    Timestamp now = Timestamp::now();
    for (WeakConnectionList::iterator it = _connectionList.begin(); it != _connectionList.end(); it++) {
        TcpConnectionPtr conn = it->lock();
        if (conn) {
            ConnInfo* info = boost::any_cast<ConnInfo>(conn->getMutableContext());
            double age = timeDifference(now, info->lastReceiveTime);
            if (age > _idleSeconds) { // timeout
                if (conn->connected()) {
                conn->shutdown();
                LOG_INFO << "timeout and shutting down connection" << conn->name();
                conn->forceCloseWithDelay(3.5);
                }
            } else if (age < 0) {
                LOG_WARN << "Time jump";
                info->lastReceiveTime = now;
            } else
                break;
        } else {
            LOG_WARN << "conn has expired";
            it = _connectionList.erase(it);
        }
    }
}

void p2pServer::onConnection(const TcpConnectionPtr & conn)
{
    LOG_INFO << conn->peerAddress().toIpPort() << " -> "
        << conn->localAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected()) {
        ConnInfo info;
        info.deviceId = string("delete");
        info.lastReceiveTime = Timestamp::now();
        _connectionList.push_back(conn);
        info.position = --_connectionList.end();
        conn->setContext(info);
    } else {
        ConnInfo* info = getConnInfo(conn);
        _connectionList.erase(info->position);
        unregisterDeviceFunc(conn);
    }
}

void p2pServer::registerDeviceFunc(const TcpConnectionPtr& conn, const string deviceId)
{
    LOG_INFO << "device id: " << deviceId;

    ConnInfo* info = getConnInfo(conn);
    info->deviceId = deviceId;

    MutexLockGuard lock(_mutex);
    ConnectionMap::iterator it = _connections.find(deviceId);
    if (it != _connections.end()) { // this time old connection is not shutdown but new register is comming
        TcpConnectionPtr oldConn = it->second; // find the old connection

        if (oldConn == conn) {
            LOG_INFO << "register again?";
            return;
        }

        ConnInfo* oldInfo = getConnInfo(oldConn);
        oldInfo->deviceId = string("delete"); // func onConnection will exec _connections.erase("delete")
        oldConn->shutdown();
        it->second = conn;
    } else
        _connections.insert(std::pair<string, TcpConnectionPtr>(deviceId, conn));
}

int32_t p2pServer::subscribeDeviceFunc(const TcpConnectionPtr& conn, const string deviceId)
{
    MutexLockGuard lock(_mutex);
    ConnectionMap::iterator it = _connections.find(deviceId);
    if (it == _connections.end())
        return EXEC_FAIL;

    TcpConnectionPtr subscribeConn = it->second;

    ConnInfo* info = getConnInfo(conn);
    if (deviceId == info->deviceId)
        return EXEC_FAIL;

    ConnInfo* subscribeInfo = getConnInfo(subscribeConn);
    info->subscribeIdListAdd(deviceId);
    subscribeInfo->sendToListAdd(conn);

    return EXEC_SUCCESS;

}

void p2pServer::unregisterDeviceFunc(const TcpConnectionPtr& conn)
{
    ConnInfo* info = getConnInfo(conn);

    while (!info->subscribeIdList.empty()) {
        unsubscribeDeviceFunc(conn, info->subscribeIdList.front()); // unsubscribe all device
    }

    info->sendToListClear(); // clear send to list

    if (info->deviceId == "delete") // means this device not register
        return;

    MutexLockGuard lock(_mutex);
    _connections.erase(info->deviceId); // erase (deviceId, conn) in connections map
    info->deviceId = string("delete"); // update connection's infomation
}

void p2pServer::unsubscribeDeviceFunc(const TcpConnectionPtr& conn, const string deviceId)
{
    ConnInfo* info = getConnInfo(conn);

    std::list<string>::iterator it = find(info->subscribeIdList.begin(), info->subscribeIdList.end(), deviceId);
    if (it != info->subscribeIdList.end()) {
        MutexLockGuard lock(_mutex);
        ConnectionMap::iterator unsubIt =_connections.find(deviceId);
        if (unsubIt != _connections.end()) {
            TcpConnectionPtr unsubscribeConn = unsubIt->second;
            ConnInfo *unsubscribeInfo = getConnInfo(unsubscribeConn);
            unsubscribeInfo->sendToListRemove(conn); // remove the conn in subscribe connection send list
        }
        info->subscribeIdList.erase(it); // remove device id in connection subscribe device id list
    }
}

int32_t p2pServer::checkDeviceAliveFunc(const string deviceId)
{
    MutexLockGuard lock(_mutex);
    ConnectionMap::iterator it = _connections.find(deviceId);
    if (it != _connections.end()) {
        if (it->second->connected())
            return EXEC_SUCCESS;
    }

    return EXEC_FAIL;
}

void p2pServer::heartBeatFunc(const TcpConnectionPtr& conn, Timestamp time)
{
    ConnInfo* info = getConnInfo(conn);
    info->lastReceiveTime = time;
    _connectionList.splice(_connectionList.end(), _connectionList, info->position);
    assert(info->position == --_connectionList.end());
}

void p2pServer::sendDataFunc(const TcpConnectionPtr& conn, const string& message)
{
    ConnInfo* info = getConnInfo(conn);

    for (std::list<TcpConnectionPtr>::iterator it = info->sendToList.begin(); it != info->sendToList.end();) {
        if ((*it)->connected()) {
            _codec.send((*it), StringPiece(message));
            it++;
        } else {
            info->sendToList.erase(it);
        }
    }
}

void p2pServer::callbackFunc(const TcpConnectionPtr& conn, const char command, const int32_t result)
{
    char callback[2] = {0};
    callback[0] = static_cast<char>(command | CALL_BACK_MASK);
    callback[1] = static_cast<char>(result);
    _codec.send(conn, StringPiece(callback, sizeof(callback)));
}

void p2pServer::onMessage(const TcpConnectionPtr& conn, const string& message, Timestamp time)
{
    switch (message[0]) {
    case REGISTER_DEVICE:
        registerDeviceFunc(conn, string(message, 1, message.length() - 1));
        callbackFunc(conn, REGISTER_DEVICE, EXEC_SUCCESS);
        break;
    case SUBSCRIBE_DEVICE:
        callbackFunc(conn, SUBSCRIBE_DEVICE, subscribeDeviceFunc(conn, string(message, 1, message.length() - 1)));
        break;
    case SEND_DATA:
        sendDataFunc(conn, message);
        callbackFunc(conn, SEND_DATA, EXEC_SUCCESS);
        break;
    case UNREGISTER_DEVICE:
        unregisterDeviceFunc(conn);
        callbackFunc(conn, UNREGISTER_DEVICE, EXEC_SUCCESS);
        break;
    case UNSUBSCRIBE_DEVICE:
        unsubscribeDeviceFunc(conn, string(message, 1, message.length() - 1));
        callbackFunc(conn, UNSUBSCRIBE_DEVICE, EXEC_SUCCESS);
        break;
    case CHECK_DEVICE_ALIVE:
        callbackFunc(conn, CHECK_DEVICE_ALIVE, checkDeviceAliveFunc(string(message, 1, message.length() - 1)));
        break;
    case HEART_BEAT:
        heartBeatFunc(conn, time);
        callbackFunc(conn, HEART_BEAT, EXEC_SUCCESS);
        break;
    default:
        LOG_INFO << "unknow command";
        break;
    }

    // LOG_INFO << message;
}
