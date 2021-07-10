#include "stupidP2P.h"
#include <unistd.h>

#define DEVICE_ID "ABCD"
#define SUBSCRIBE_DEVICE_ID "FFSS"

int main(int argc, char *argv[])
{
    printf("pid: %d\n", getpid());
    int32_t ret = 0;
    ret = stupid_p2p_init("127.0.0.1", 4040);
    printf("ret: 0x%x\n", ret);
    int32_t fd = ret;

    sleep(1);

    ret = stupid_p2p_register_device(fd, DEVICE_ID);
    printf("ret: 0x%x\n", ret);
    ret = stupid_p2p_subscribe_device(fd, SUBSCRIBE_DEVICE_ID);
    printf("ret: 0x%x\n", ret);
    ret = stupid_p2p_send_data(fd, "fuck", 4);
    printf("ret: 0x%x\n", ret);
    ret = stupid_p2p_unregister_device(fd);
    printf("ret: 0x%x\n", ret);
    ret = stupid_p2p_unsubscribe_device(fd, SUBSCRIBE_DEVICE_ID);
    printf("ret: 0x%x\n", ret);
    ret = stupid_p2p_check_device_alive(fd, SUBSCRIBE_DEVICE_ID);
    printf("ret: 0x%x\n", ret);
    ret = stupid_p2p_heart_beat(fd);
    printf("ret: 0x%x\n", ret);
    

    sleep(10);
    stupid_p2p_deinit(fd);
}
