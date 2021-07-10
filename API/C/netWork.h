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
#define NET_SEND_COMMAND_NUM    8192

/*
    net package format:
        package length(4 bytes) + protocol command and data
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
            receive data: 0x03 + data
*/
#define     NET_PACKAGE_LENGTH              0x04
#define     CALL_BACK_MASK                  0x80

#define     REGISTER_DEVICE                 0x01
#define     REGISTER_DEVICE_CALL_BACK       (CALL_BACK_MASK | REGISTER_DEVICE)
#define     SUBSCRIBE_DEVICE                0x02
#define     SUBSCRIBE_DEVICE_CALL_BACK      (CALL_BACK_MASK | SUBSCRIBE_DEVICE)
#define     SEND_DATA                       0x03
#define     SEND_DATA_CALL_BACK             (CALL_BACK_MASK | SEND_DATA)
#define     UNREGISTER_DEVICE               0x04
#define     UNREGISTER_DEVICE_CALL_BACK     (CALL_BACK_MASK | UNREGISTER_DEVICE)
#define     UNSUBSCRIBE_DEVICE              0x05
#define     UNSUBSCRIBE_DEVICE_CALL_BACK    (CALL_BACK_MASK | UNSUBSCRIBE_DEVICE)
#define     CHECK_DEVICE_ALIVE              0x06
#define     CKECK_DEVICE_ALIVE_CALL_BACK    (CALL_BACK_MASK | CHECK_DEVICE_ALIVE)
#define     HEART_BEAT                      0x07
#define     HEART_BEAT_CALL_BACK            (CALL_BACK_MASK | HEART_BEAT)
#define     EXEC_FAIL                       0x00
#define     EXEC_SUCCESS                    0x01

// represent command status
// 0x0: error, 0x1: success, 0xff: not receive
#define CALL_BACK_ERROR         0x00
#define CALL_BACK_SUCCESS       0x01
#define CALL_BACK_NOT_RECEIVE   0xff

typedef struct {
    uint8_t register_flag;
    uint8_t subscribe_flag;
    uint8_t send_data_flag;
    uint8_t unregister_flag;
    uint8_t unsubscribe_flag;
    uint8_t check_alive_flag;
    uint8_t heart_beat_flag;
    uint8_t heart_beat_not_receive_num;
} realtime_command_status_t;

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
static inline void free_recv_data_t(recv_data_t *recv_data)
{
    printf("free recv_data");
    if (recv_data->data)
        free(recv_data->data);
    free(recv_data);
}

static inline void recv_data_list_add(recv_data_list_t *list, recv_data_list_node_t *node)
{
    pthread_mutex_lock(&list->recv_data_list_mutex);
    recv_data_list_node_t *prev = list->recv_data_head.prev;
    node->next = prev->next;
    node->prev = prev;
    prev->next = node;
    prev->next->prev = node;
    pthread_mutex_unlock(&list->recv_data_list_mutex);
}

static inline recv_data_t* recv_data_list_return_data(recv_data_list_t *list)
{
    pthread_mutex_lock(&list->recv_data_list_mutex);
    recv_data_list_node_t *node = list->recv_data_head.next;

    if (node == &list->recv_data_head) {
        pthread_mutex_unlock(&list->recv_data_list_mutex);
        return NULL;
    }
    recv_data_t *data = node->recv_data;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node); 
    pthread_mutex_unlock(&list->recv_data_list_mutex);

    return data;
}

static inline void recv_data_list_deinit(recv_data_list_t *list)
{
    pthread_mutex_lock(&list->recv_data_list_mutex);
    while (list->recv_data_head.next != &(list->recv_data_head)) {
        recv_data_list_node_t *node = list->recv_data_head.next;

        free_recv_data_t(node->recv_data);
        node->prev->next = node->next;
        node->next->prev = node->prev;
        free(node);
    }
    pthread_mutex_unlock(&list->recv_data_list_mutex);

    pthread_mutex_destroy(&list->recv_data_list_mutex);
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


typedef struct {
    uint8_t *data;
    int32_t size;
} net_work_command_t;

typedef struct {
    net_work_command_t command[NET_SEND_COMMAND_NUM];
    net_work_command_t* r_ptr;
    net_work_command_t* w_ptr;
    int32_t num;
    pthread_mutex_t send_data_fifo_mutex;
} net_work_send_command_fifo_t;

static inline int32_t send_command_fifo_init(net_work_send_command_fifo_t *fifo)
{
    fifo->r_ptr = fifo->w_ptr = fifo->command;
    fifo->num = 0;
    return pthread_mutex_init(&fifo->send_data_fifo_mutex, NULL);
}

static inline void send_command_fifo_push(net_work_send_command_fifo_t *fifo, uint8_t *data, int32_t size)
{
    pthread_mutex_lock(&fifo->send_data_fifo_mutex);
    if(fifo->w_ptr->data != NULL)
        free(fifo->w_ptr->data);

    fifo->w_ptr->data = data;
    fifo->w_ptr->size = size;


    if (fifo->w_ptr == &fifo->command[NET_SEND_COMMAND_NUM - 1])
        fifo->w_ptr = fifo->command;
    else
        fifo->w_ptr++;

    fifo->num++;

    pthread_mutex_unlock(&fifo->send_data_fifo_mutex);
}

static inline bool send_command_fifo_empty(net_work_send_command_fifo_t *fifo)
{
    return (fifo->num == 0);
}

static inline int32_t send_command_fifo_length(net_work_send_command_fifo_t *fifo)
{
    return fifo->num;
}

static inline bool send_command_fifo_full(net_work_send_command_fifo_t *fifo)
{
    return (fifo->num == NET_SEND_COMMAND_NUM);
}

static inline void send_command_fifo_pop(net_work_send_command_fifo_t *fifo)
{
    pthread_mutex_unlock(&fifo->send_data_fifo_mutex);
    free(fifo->r_ptr->data);

    if (fifo->r_ptr == &fifo->command[NET_SEND_COMMAND_NUM - 1])
        fifo->r_ptr = fifo->command;
    else
        fifo->r_ptr++;

    fifo->num--;
    pthread_mutex_unlock(&fifo->send_data_fifo_mutex);
}

static inline net_work_command_t *send_command_fifo_top(net_work_send_command_fifo_t *fifo)
{
    return fifo->r_ptr;
}

static inline void send_command_fifo_deinit(net_work_send_command_fifo_t *fifo)
{
    while (!send_command_fifo_empty(fifo)) {
        net_work_command_t *command = send_command_fifo_top(fifo);
        free(command->data);
        send_command_fifo_pop(fifo);
    }
    pthread_mutex_destroy(&fifo->send_data_fifo_mutex);
}


struct stupid_p2p_client_node {
    int32_t fd;
    int32_t socket_fd;
    struct sockaddr_in server_addr;

    net_work_recv_data_fifo_t  recv_fifo;
    net_work_send_command_fifo_t send_fifo;
    recv_data_list_t recv_data_list;

    pthread_t stupid_client_receive_id;
    pthread_t stupid_client_send_id;
    pthread_t stupid_client_check_net_id;

    int32_t need_to_restart_flag;

    realtime_command_status_t command_status;

    struct stupid_p2p_client_node *next;
};
typedef struct stupid_p2p_client_node stupid_p2p_t;

void parse_recv_net_command(stupid_p2p_t *stupid_p2p);

#ifdef __cplusplus
}
#endif

#endif