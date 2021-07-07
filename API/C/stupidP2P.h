#ifndef __STUPID_P2P__
#define __STUPID_P2P__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define USE_OUT_LOG_MODULE  0
#if USE_OUT_LOG_MODULE
    // implement _log
#else
#include <stdio.h>
#define _log(fmt, ...) do {   \
    printf(fmt, ##__VA_ARGS__);     \
    } while(0)
#endif

enum {
    STUPID_P2P_NO_ERROR             = 0,
    STUPID_P2P_ERROR_COMMOND        = 0x80000001,
    STUPID_P2P_BAD_PARAM,
    STUPID_P2P_MEMORY_ERROR
};

int32_t stupid_p2p_init(const char *server_ip, const int32_t server_port);
void stupid_p2p_deinit(int32_t fd);



#ifdef __cplusplus
}
#endif

#endif