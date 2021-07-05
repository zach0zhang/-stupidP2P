#include "p2pServer.h"

#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"

#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

int main(int argc, char* argv[])
{
    LOG_INFO << "pid = " << getpid();

    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress serverAddr(port);

    p2pServer server(&loop, serverAddr, 300);
    server.start();
    loop.loop();
}
