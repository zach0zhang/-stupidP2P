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



#ifdef __cplusplus
}
#endif

#endif