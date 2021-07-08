#ifndef __NET_WORK_H__
#define __NET_WORK_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>

#define NET_READ_DATA_LENGTH    8192

// represent command status
// 0x0: error, 0x1: success, 0xff: not receive
typedef struct {
    uint8_t register_flag;
    uint8_t subscribe_flag;
    uint8_t send_data_flag;
    uint8_t unregister_flag;
    uint8_t unsubscribe_flag;
    uint8_t check_alive_flag;
    uint8_t heart_beat_flag;
    uint8_t not_receive_num;
} realtime_command_status_t;


typedef struct {
    int32_t length;
    uint8_t *data;
}recv_data_t;

struct recv_data_list_node{
    recv_data_t *recv_data;
    struct recv_data_list_node *next;
    struct recv_data_list_node *prev;
};
typedef struct recv_data_list_node recv_data_list_node_t;

typedef struct {
    recv_data_list_node_t recv_data_head;
    pthread_mutex_t recv_data_list_mutex;
}recv_data_list_t;

static inline int32_t recv_data_list_init(recv_data_list_t *list)
{
    int32_t ret = 0;
    
    recv_data_list_node_t *node = &list->recv_data_head;
    node->next = node;
    node->prev = node;

    ret = pthread_mutex_init(&list->recv_data_list_mutex, NULL);

    return ret;
}

static inline void recv_data_list_deinit(recv_data_list_t *list)
{
    pthread_mutex_destroy(&list->recv_data_list_mutex);
}

static inline void recv_data_list_add(recv_data_list_t *list, recv_data_list_node_t *node)
{
    pthread_mutex_lock(&list->recv_data_list_mutex);
    recv_data_list_node_t *prev = list->recv_data_head.prev;
    node->next = prev->next;
    node->prev = prev;
    prev->next = node;
    pthread_mutex_unlock(&list->recv_data_list_mutex);
}

static inline void recv_data_list_remove(recv_data_list_t *list, recv_data_list_node_t *node)
{
    pthread_mutex_lock(&list->recv_data_list_mutex);
    node->prev->next = node->next;
    node->next->prev = node->prev;
    pthread_mutex_unlock(&list->recv_data_list_mutex);
    free(node);
}


typedef struct {
    uint8_t data[NET_READ_DATA_LENGTH];
    uint8_t* r_ptr;
    uint8_t* w_ptr;
    int32_t length;
} net_work_recv_data_fifo_t;

static inline void recv_data_fifo_init(net_work_recv_data_fifo_t *fifo)
{
    fifo->r_ptr = fifo->w_ptr = fifo->data;
    fifo->length = 0;
}

static inline void recv_data_fifo_push(net_work_recv_data_fifo_t *fifo, const uint8_t * const data, const int32_t length)
{
    for (int i = 0; i < length; i++) {
        *(fifo->w_ptr) = data[i];
        if (fifo->w_ptr == &fifo->data[NET_READ_DATA_LENGTH - 1])
            fifo->w_ptr = fifo->data;
        else
            fifo->w_ptr++; 
    }

    fifo->length += length;
}

static inline bool recv_data_fifo_empty(net_work_recv_data_fifo_t *fifo)
{
    return (fifo->length == 0);
}

static inline int32_t recv_data_fifo_length(net_work_recv_data_fifo_t *fifo)
{
    return fifo->length;
}

static inline bool recv_data_fifo_full(net_work_recv_data_fifo_t *fifo)
{
    return (fifo->length == NET_READ_DATA_LENGTH);
}

static inline void recv_data_fifo_pop(net_work_recv_data_fifo_t *fifo, const int32_t length)
{
    for (int i = 0; i < length; i++) {
        if (fifo->r_ptr == &fifo->data[NET_READ_DATA_LENGTH - 1])
            fifo->r_ptr = fifo->data;
        else
            fifo->r_ptr++;
    }
    fifo->length -= length;
}

static inline uint8_t *recv_data_fifo_top(net_work_recv_data_fifo_t *fifo)
{
    return fifo->r_ptr;
}

static inline int32_t recv_data_fifo_top_int(net_work_recv_data_fifo_t *fifo)
{
    int32_t ret = 0;
    uint8_t *ptr = fifo->r_ptr;
    for (int32_t i = 3; i >= 0; i--) {
        ret += (*ptr) << (i * 8); 
        if (ptr == &fifo->data[NET_READ_DATA_LENGTH - 1])
            ptr = fifo->data;
        else
            ptr++;
    }

    return ret;
}


struct stupid_p2p_client_node {
    int32_t fd;
    int32_t socket_fd;
    struct sockaddr_in server_addr;

    net_work_recv_data_fifo_t  recv_fifo;

    pthread_t stupid_client_receive_id;
    pthread_t stupid_client_send_id;

    int32_t need_to_restart_flag;

    realtime_command_status_t command_status;
    
    recv_data_list_t recv_data_list;

    struct stupid_p2p_client_node *next;
};
typedef struct stupid_p2p_client_node stupid_p2p_t;

void parse_recv_net_command(stupid_p2p_t *stupid_p2p);

#ifdef __cplusplus
}
#endif

#endif