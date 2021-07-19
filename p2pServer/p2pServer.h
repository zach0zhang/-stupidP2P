#ifndef __P2P_SERVER__
#define __P2P_SERVER__

#include "codec.h"

#include "muduo/base/Mutex.h"
#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"


#include <algorithm>
#include <list>
#include <map>

class p2pServer
{
    public:

        /*
            protocol format:
                command(ProtocolCode) + data(send data or device id):
                    register device: 0x01 + device_id
                    subscribe device: 0x02 + device_id
                    send data: 0x03 + data
                    unregister_device: 0x04
                    unsubscribe_device: 0x05 + device_id
                    check device alive: 0x06

                callback format:
                command(ProtocolCode) | CALL_BACK_MASK(0x80) + SUCCESS/FAIL
        */
        enum ProtocolCode {
            REGISTER_DEVICE     = 0x01,
            SUBSCRIBE_DEVICE    = 0x02,
            SEND_DATA           = 0x03,
            UNREGISTER_DEVICE   = 0x04,
            UNSUBSCRIBE_DEVICE  = 0x05,
            CHECK_DEVICE_ALIVE  = 0x06,
            HEART_BEAT          = 0x07,
            CALL_BACK_MASK      = 0x80
        };

        enum CallBackStatus {
            EXEC_FAIL           = 0x0,
            EXEC_SUCCESS        = 0x01
        };

        p2pServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr, int idleSeconds);
        void start();

    protected:
        void registerDeviceFunc(const muduo::net::TcpConnectionPtr& conn, const muduo::string deviceId);
        int32_t subscribeDeviceFunc(const muduo::net::TcpConnectionPtr& conn, const muduo::string deviceId);
        void unregisterDeviceFunc(const muduo::net::TcpConnectionPtr& conn);
        void sendDataFunc(const muduo::net::TcpConnectionPtr& conn, const muduo::string& message);
        void unsubscribeDeviceFunc(const muduo::net::TcpConnectionPtr& conn, const muduo::string deviceId);
        int32_t checkDeviceAliveFunc(const muduo::string deviceId);
        void heartBeatFunc(const muduo::net::TcpConnectionPtr& conn, muduo::Timestamp time);
        void callbackFunc(const muduo::net::TcpConnectionPtr& conn, const char command, const int result);
    private:
        typedef std::map<muduo::string, muduo::net::TcpConnectionPtr> ConnectionMap;

        typedef std::weak_ptr<muduo::net::TcpConnection> WeakTcpConnectionPtr;
        typedef std::list<WeakTcpConnectionPtr> WeakConnectionList;
        struct ConnInfo : public muduo::copyable
        {
            muduo::string deviceId;
            muduo::Timestamp lastReceiveTime;
            WeakConnectionList::iterator position;
            std::list<muduo::string> subscribeIdList;
            std::list<muduo::net::TcpConnectionPtr> sendToList;
            void subscribeIdListAdd(const muduo::string id)
            {
                if (find(this->subscribeIdList.begin(), this->subscribeIdList.end(), id) == this->subscribeIdList.end())
                    this->subscribeIdList.push_back(id);
            }

            void subscribeIdListClear() {
                this->subscribeIdList.clear();
            }

            void sendToListAdd(const muduo::net::TcpConnectionPtr& conn) {
                if (std::find(this->sendToList.begin(), this->sendToList.end(), conn) == this->sendToList.end())
                    this->sendToList.push_back(conn);
            }

            void sendToListRemove(const muduo::net::TcpConnectionPtr& conn) {
                this->sendToList.remove(conn);
            }

            void sendToListClear() {
                this->sendToList.clear();
            }
        };
        ConnInfo* getConnInfo(const muduo::net::TcpConnectionPtr& conn);

        void onConnection(const muduo::net::TcpConnectionPtr& conn);
        void onMessage(const muduo::net::TcpConnectionPtr & conn, const muduo::string& message, muduo::Timestamp);
        void onTimer();

        muduo::net::TcpServer _server;
        LengthHeaderCodec _codec;
        int _idleSeconds;
        WeakConnectionList _connectionList;
        muduo::MutexLock _mutex;
        ConnectionMap _connections GUARDED_BY(_mutex);
};




#endif