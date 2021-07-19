# stupidP2P
stupidP2P（笨比P2P），正如其名，基于Socket完成了一个简单且低效的P2P通信项目：使用一个server作为中继，实现多个client互相进行数据通信。
工程分为两部分，一部分是作为中继的server端，使用C++实现了一个根据client注册和订阅的Id号来接收和转发数据的服务器程序，并通过Tcp连接和提供内部接口与client进行功能交互；另一部分是client端，以对外API接口的方式呈现，接口内部自动维护与server端的交互，并实现断线自动重连，目前API支持C(Linux)与Python，通过API可以实现利用server进行中继的client。

## 1. server
server端([p2pServer](https://github.com/zach0zhang/stupidP2P/tree/master/p2pServer))使用C++与网络库[muduo](https://github.com/chenshuo/muduo)实现，并定义了client与server交互的命令格式：

```
protocol format:
    send format:
        command(ProtocolCode) + data(send data or device id):
            register device: 0x01 + device_id
            subscribe device: 0x02 + device_id
            send data: 0x03 + data
            unregister_device: 0x04
            unsubscribe_device: 0x05 + device_id
            check device alive: 0x06

    callback format:
        command(ProtocolCode) | CALL_BACK_MASK(0x80) + SUCCESS/FAIL(0x1/0x0)
```

编译：
```
cd p2pServer
make release
```

执行： ./p2pServer 端口，例如server启用4040端口监听client命令：
```
./p2pServer 4040
```

## 2. client API
### 2.1 C

[C的API接口](https://github.com/zach0zhang/stupidP2P/tree/master/API/C)使用Linux Socket编程实现与server的通信，实现server端定义的命令来进行交互功能，api内部维护与server端的Tcp连接，收发和解析命令，并与server端定时发送心跳，检测网络并且重连，对外提供实现client必须的C接口（在linux环境下）。

api接口函数：
```C
enum {
    STUPID_P2P_NO_ERROR             = 0,
    STUPID_P2P_ERROR_COMMOND        = 0x80000001,
    STUPID_P2P_BAD_PARAM,
    STUPID_P2P_MEMORY_ERROR,
    STUPID_P2P_RESTARTING,
    STUPID_P2P_NET_ERROR
};

int32_t stupid_p2p_init(const uint8_t *server_ip, const int32_t server_port);
void stupid_p2p_deinit(int32_t fd);
int32_t stupid_p2p_register_device(const int32_t fd, const uint8_t *device_id);
int32_t stupid_p2p_subscribe_device(const int32_t fd, const uint8_t *device_id);
int32_t stupid_p2p_send_data(const int32_t fd, uint8_t *buf, int32_t length);
int32_t stupid_p2p_unregister_device(const int32_t fd);
int32_t stupid_p2p_unsubscribe_device(const int32_t fd, const uint8_t *device_id);
int32_t stupid_p2p_check_device_alive(const int32_t fd, const uint8_t *device_id);
int32_t stupid_p2p_heart_beat(const int32_t fd); // Internal function no need to call

typedef struct {
    int32_t length;
    uint8_t *data;
}recv_data_t;

typedef struct {
    int32_t num;
    recv_data_t **recv_data;
}recv_data_buf_t;

recv_data_buf_t *stupid_p2p_recv_data(const int32_t fd);
void stupid_p2p_free_recv_data_buf(recv_data_buf_t *recv_data_buf);
```

根据C的API实现的client测试程序编译及执行：
```
编译：
    cd ./API/C/test
    make

执行：
    ./p2pClient -s server_ip -p server_port -m my_id -o subscribe_id

例如想让id为"client1"和"client2"的两个client通过server(192.168.11.63:4040)中继进行通信，则分别执行（已经启动了server端程序）:
    ./p2pClient -s 192.168.11.63 -p 4040 -m client1 -o client2
    ./p2pClient -s 192.168.11.63 -p 4040 -m client2 -o client1

```

![api_c_test](https://github.com/zach0zhang/stupidP2P/tree/master/doc/pic/api_c_test.png)

### 2.2 Python
[Python的API接口](https://github.com/zach0zhang/stupidP2P/tree/master/API/Python)封装了一个stupidP2PClient类，并提供了与C接口函数功能基本一致的方法，类内通过socket维护与server通信的Tcp，定时发送心跳，检测断线自动重连

使用方法：

```Python
# 使用server(192.168.11.63:4040)作为中继服务器，注册本机id "client1", 订阅id "client2"
client = stupidP2PClient("192.168.11.63", 4040)
client.initClient()
client.registerDevice("client1")
client.checkDeviceAlive("client2")
client.subscribeDevice("clinet2")
client.sendData("hello")
dataList = client.recvData()
client.unscribeDevice("client2")
client.unregisterDevice()
client.deinitClient()
```

测试Python接口，使用Pyqt5+stupidP2PClient实现了一个通讯[client](https://github.com/zach0zhang/stupidP2P/tree/master/API/Python/test):

![api_python_test](https://github.com/zach0zhang/stupidP2P/tree/master/doc/pic/api_python_test.gif)

