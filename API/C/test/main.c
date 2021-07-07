#include "stupidP2P.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
    int32_t ret = 0;
    ret = stupid_p2p_init("192.168.2.189", 4040);
    printf("ret: %d\n", ret);

    sleep(10);

    stupid_p2p_deinit(ret);
}
