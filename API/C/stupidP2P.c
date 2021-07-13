#include "stupidP2P.h"
#include "netWork.h"

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

stupid_p2p_t stupid_p2p_head = {0};
pthread_rwlock_t stupid_p2p_rwlock = PTHREAD_RWLOCK_INITIALIZER;

static volatile int32_t _g_fd_start = 0;
static int32_t _g_init_signal_flag = 0;

static inline int32_t add_stupid_p2p_to_list(stupid_p2p_t *node)
{
    pthread_rwlock_wrlock(&stupid_p2p_rwlock);
    stupid_p2p_t *p = &stupid_p2p_head;
    int32_t ret = ++_g_fd_start;
    while (p->next != NULL)
        p = p->next;
    p->next = node;
    pthread_rwlock_unlock(&stupid_p2p_rwlock);
    return ret;
}

static inline void remove_stupid_p2p(int32_t fd)
{
    pthread_rwlock_wrlock(&stupid_p2p_rwlock);
    stupid_p2p_t *p, *prev;
    p = prev = &stupid_p2p_head;
    while (p->next != NULL) {
        if (p->fd == fd) {
            prev->next = p->next;
            free(p);
            break;
        }
        prev = p;
        p = p->next;
    }
    pthread_rwlock_unlock(&stupid_p2p_rwlock);
}

static inline stupid_p2p_t *find_stupid_p2p(int32_t fd)
{
    pthread_rwlock_rdlock(&stupid_p2p_rwlock);
    stupid_p2p_t *p = &stupid_p2p_head;

    while (p != NULL) {
        if (p->fd == fd)
            break;
        p = p->next;
    }
    pthread_rwlock_unlock(&stupid_p2p_rwlock);
    return p;
}

void sig_handle(int sig)
{
    _log("the program get sig %d\n", sig);
}

static void check_module_init()
{
    if (!_g_init_signal_flag) {
        signal(SIGPIPE, sig_handle);
        _g_init_signal_flag = 1;
    }
}

static int32_t waitting_flag_and_return(uint8_t *flag)
{
    const static int32_t waiting_3s = 300;
    int32_t repeat = 0;

    if (flag == NULL)
        return STUPID_P2P_BAD_PARAM;
    
    while(1) {
        if (*flag != CALL_BACK_NOT_RECEIVE || repeat >= waiting_3s)
            break;
        repeat++;
        usleep(10000);
    }

    if (*flag == CALL_BACK_SUCCESS)
        return STUPID_P2P_NO_ERROR;
    else if (*flag == CALL_BACK_NOT_RECEIVE)
        return STUPID_P2P_NET_ERROR;
    else
        return STUPID_P2P_ERROR_COMMOND;
}

static inline int32_t need_to_send(const int32_t fd, uint8_t command, const uint8_t *data, int32_t length)
{
    if (length > 256 || fd <= 0)
        return STUPID_P2P_BAD_PARAM;
        
    stupid_p2p_t *stupid_p2p = find_stupid_p2p(fd);
    if (stupid_p2p == NULL) {
        _log("can't find fd\n");
        return STUPID_P2P_ERROR_COMMOND;
    }

    if (stupid_p2p->need_to_restart_flag == 1)
        return STUPID_P2P_RESTARTING;

    uint8_t *send_data = (uint8_t *)malloc(sizeof(uint8_t) * (length + 1));
    if (send_data == NULL)
        return STUPID_P2P_MEMORY_ERROR;

    send_data[0] = command;
    memcpy(&send_data[1], data, length);

    uint8_t *flag = NULL;
    switch (command) {
        case REGISTER_DEVICE:
            flag = &(stupid_p2p->command_status.register_flag);
            break;
        case SUBSCRIBE_DEVICE:
            flag = &(stupid_p2p->command_status.subscribe_flag);
            break;
        case SEND_DATA:
            flag = &(stupid_p2p->command_status.send_data_flag);
            break;
        case UNREGISTER_DEVICE:
            flag = &(stupid_p2p->command_status.unregister_flag);
            break;
        case UNSUBSCRIBE_DEVICE:
            flag = &(stupid_p2p->command_status.unsubscribe_flag);
            break;
        case CHECK_DEVICE_ALIVE:
            flag = &(stupid_p2p->command_status.check_alive_flag);
            break;
        case HEART_BEAT:
            flag = &(stupid_p2p->command_status.heart_beat_flag);
            break;
        default:
            free(send_data);
            return STUPID_P2P_BAD_PARAM;
    }

    *flag = CALL_BACK_NOT_RECEIVE;
    send_command_fifo_push(&stupid_p2p->send_fifo, send_data, length + 1);

    return waitting_flag_and_return(flag);
}

static void *_stupid_p2p_recv_func(void *argv)
{
    pthread_detach(pthread_self());
    
    stupid_p2p_t *stupid_p2p = (stupid_p2p_t *)argv;
    int32_t ret;
    uint8_t buf[4096] = {0};
    uint8_t thread_name[64] = {0};

    snprintf(thread_name, sizeof(thread_name), "fd:%d:recv thread", stupid_p2p->fd);
    prctl(PR_SET_NAME, thread_name);
    
    while (1) {
        if (stupid_p2p->need_to_restart_flag) {
            sleep(1);
            continue;
        }
       
        ret = recv(stupid_p2p->socket_fd, buf, sizeof(buf), 0);
        if (ret <= 0 && errno != EINTR) {
            _log("fd: %d receive error, maybe connection is close\n", stupid_p2p->fd);
            stupid_p2p->need_to_restart_flag = 1;
            continue;
        } else if (ret + recv_data_fifo_length(&stupid_p2p->recv_fifo) > NET_READ_DATA_LENGTH){
            _log("fd: %d receive fifo is full\n", stupid_p2p->fd);
            // TODO: exec fifo is full
        } else {
            recv_data_fifo_push(&stupid_p2p->recv_fifo, buf, ret);
        }

        parse_recv_net_command(stupid_p2p);
    }

    return NULL;
}

static void prepend_length_to_buf_s(uint8_t *dst, int32_t dst_length, uint8_t *src, int32_t src_length)
{
    src_length = (src_length > dst_length - 4) ? dst_length - 4 : src_length;

    int32_t i = 0;
    for (i = 0; i < 4; i++) {
        dst[i] = (uint8_t)(src_length >> (8 * (3 - i))) & 0xff;
    }

    memcpy(&dst[i], src, src_length);
}

static void *_stupid_p2p_send_func(void *argv)
{
    pthread_detach(pthread_self());
    
    stupid_p2p_t *stupid_p2p = (stupid_p2p_t *)argv;
    int32_t ret = 0;
    uint8_t buf[2048] = {0};
    uint8_t thread_name[64] = {0};

    snprintf(thread_name, sizeof(thread_name), "fd:%d:send thread", stupid_p2p->fd);
    prctl(PR_SET_NAME, thread_name);
    
    while(1) {
        while (!send_command_fifo_empty(&stupid_p2p->send_fifo)) {
            if (stupid_p2p->need_to_restart_flag) {
                sleep(1);
                break;
            }
            net_work_command_t *command = send_command_fifo_top(&stupid_p2p->send_fifo);
            prepend_length_to_buf_s(buf, sizeof(buf), command->data, command->size);
            ret = send(stupid_p2p->socket_fd, buf, command->size + 4, 0);
            if (ret != command->size + 4)
                _log("need send %d bytes, now send %d bytes\n", command->size + 4, ret);
            send_command_fifo_pop(&stupid_p2p->send_fifo);
        }
        usleep(50000);
    }

    return NULL;
}

static int32_t start_connection(stupid_p2p_t *stupid_p2p)
{
    stupid_p2p->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (stupid_p2p->socket_fd < 0) {
        _log("%s: Failed to Create client socket\n", __func__);
        return STUPID_P2P_ERROR_COMMOND;
    }

    if (connect(stupid_p2p->socket_fd, (struct sockaddr *)&stupid_p2p->server_addr, sizeof(stupid_p2p->server_addr))) {
        close(stupid_p2p->socket_fd);
        _log("%s: %s\n", __func__, strerror(errno));
        return STUPID_P2P_ERROR_COMMOND;
    }

    return STUPID_P2P_NO_ERROR;
}

static void *_stupid_p2p_check_net_func(void *argv)
{
    pthread_detach(pthread_self());

    const static int32_t heart_beat_max = 3;
    stupid_p2p_t *stupid_p2p = (stupid_p2p_t *)argv;
    uint8_t thread_name[64] = {0};
    int32_t ret = STUPID_P2P_NO_ERROR;

    snprintf(thread_name, sizeof(thread_name), "fd:%d:check net thread", stupid_p2p->fd);
    prctl(PR_SET_NAME, thread_name);

    while(1) {
        if (stupid_p2p->need_to_restart_flag) {
            _log("fd:%d restart net now\n", stupid_p2p->fd);
            close(stupid_p2p->socket_fd);
            ret = start_connection(stupid_p2p);
            if (ret != STUPID_P2P_NO_ERROR) {
                _log("fd:%d can not connect to server try again\n", stupid_p2p->fd);
                sleep(1);
                continue;
            }
            _log("fd%d success to connection the server, socket_fd: %d\n", stupid_p2p->fd, stupid_p2p->socket_fd);
            stupid_p2p->need_to_restart_flag = 0;
            stupid_p2p->command_status.heart_beat_not_receive_num = 0;
        }

        ret = need_to_send(stupid_p2p->fd, HEART_BEAT, NULL, 0);
        if (ret == STUPID_P2P_NO_ERROR) {
            stupid_p2p->command_status.heart_beat_not_receive_num = 0;
        } else if (ret = STUPID_P2P_NET_ERROR) {
            stupid_p2p->command_status.heart_beat_not_receive_num++;
            _log("fd:%d get net error, heart beat not receive num: %d\n", stupid_p2p->fd, stupid_p2p->command_status.heart_beat_not_receive_num);
        }

        if (stupid_p2p->command_status.heart_beat_not_receive_num >= heart_beat_max)
            stupid_p2p->need_to_restart_flag = 1;

        sleep(3);
    }
    
    return NULL;
}

int32_t stupid_p2p_init(const uint8_t *server_ip, const int32_t server_port)
{
    int32_t ret = STUPID_P2P_NO_ERROR;

    check_module_init();

    stupid_p2p_t *stupid_p2p = (stupid_p2p_t *)calloc(1, sizeof(stupid_p2p_t));
    if (stupid_p2p == NULL) {
        _log("%s: calloc error\n", __func__);
        return STUPID_P2P_MEMORY_ERROR;
    }

    stupid_p2p->server_addr.sin_family = AF_INET;
    stupid_p2p->server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &stupid_p2p->server_addr.sin_addr) == 0) {
        _log("%s: Invalid IP address\n", __func__);
        return STUPID_P2P_BAD_PARAM;
    }
    recv_data_fifo_init(&stupid_p2p->recv_fifo);
    recv_data_list_init(&stupid_p2p->recv_data_list);
    send_command_fifo_init(&stupid_p2p->send_fifo);

    ret = start_connection(stupid_p2p);
    if (ret != STUPID_P2P_NO_ERROR)
        return ret;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x400000);
    ret = pthread_create(&stupid_p2p->stupid_client_receive_id, &attr, _stupid_p2p_recv_func, (void *)stupid_p2p);
    if (ret != 0) {
        _log("%s: create receive thread error\n", __func__);
        close(stupid_p2p->socket_fd);
        return STUPID_P2P_ERROR_COMMOND;
    }
    ret = pthread_create(&stupid_p2p->stupid_client_send_id, &attr, _stupid_p2p_send_func, (void *)stupid_p2p);
    if (ret != 0) {
        _log("%s: create send thread error\n", __func__);
        pthread_cancel(stupid_p2p->stupid_client_receive_id);
        pthread_join(stupid_p2p->stupid_client_receive_id, NULL);
        close(stupid_p2p->socket_fd);
        return STUPID_P2P_ERROR_COMMOND;
    }
    ret = pthread_create(&stupid_p2p->stupid_client_check_net_id, &attr, _stupid_p2p_check_net_func, (void *)stupid_p2p);
    if (ret != 0) {
        _log("%s: crete check net thread error\n", __func__);
        pthread_cancel(stupid_p2p->stupid_client_receive_id);
        pthread_join(stupid_p2p->stupid_client_receive_id, NULL);
        pthread_cancel(stupid_p2p->stupid_client_send_id);
        pthread_join(stupid_p2p->stupid_client_send_id, NULL);
        close(stupid_p2p->socket_fd);
        return STUPID_P2P_ERROR_COMMOND;
    }

    stupid_p2p->fd = add_stupid_p2p_to_list(stupid_p2p);
    return stupid_p2p->fd;
}

void stupid_p2p_deinit(int32_t fd)
{
    stupid_p2p_t *stupid_p2p = find_stupid_p2p(fd);
    if (stupid_p2p == NULL)
        return;

    pthread_cancel(stupid_p2p->stupid_client_receive_id);
    pthread_cancel(stupid_p2p->stupid_client_send_id);
    pthread_cancel(stupid_p2p->stupid_client_check_net_id);
    pthread_join(stupid_p2p->stupid_client_receive_id, NULL);
    pthread_join(stupid_p2p->stupid_client_send_id, NULL);
    pthread_join(stupid_p2p->stupid_client_check_net_id, NULL);

    close(stupid_p2p->socket_fd);
    recv_data_list_deinit(&stupid_p2p->recv_data_list);
    send_command_fifo_deinit(&stupid_p2p->send_fifo);
    remove_stupid_p2p(stupid_p2p->fd);
}

int32_t stupid_p2p_register_device(const int32_t fd, const uint8_t *device_id)
{
    return need_to_send(fd, REGISTER_DEVICE, device_id, strnlen(device_id, 256));
}

int32_t stupid_p2p_subscribe_device(const int32_t fd, const uint8_t *device_id)
{
    return need_to_send(fd, SUBSCRIBE_DEVICE, device_id, strnlen(device_id, 256));
}

int32_t stupid_p2p_send_data(const int32_t fd, uint8_t *buf, int32_t length)
{
    return need_to_send(fd, SEND_DATA, buf, length);
}

int32_t stupid_p2p_unregister_device(const int32_t fd)
{
    return need_to_send(fd, UNREGISTER_DEVICE, NULL, 0);
}

int32_t stupid_p2p_unsubscribe_device(const int32_t fd, const uint8_t *device_id)
{
    return need_to_send(fd, UNSUBSCRIBE_DEVICE, device_id, strnlen(device_id, 256));
}

int32_t stupid_p2p_check_device_alive(const int32_t fd, const uint8_t *device_id)
{
    return need_to_send(fd, CHECK_DEVICE_ALIVE, device_id, strnlen(device_id, 256));
}

int32_t stupid_p2p_heart_beat(const int32_t fd)
{
    return need_to_send(fd, HEART_BEAT, NULL, 0);
}

recv_data_buf_t *stupid_p2p_recv_data(const int32_t fd)
{
    const static int32_t max_num = 1024;
    int32_t recv_data_num = 0;
    recv_data_t *data_buf[1024] = {0};

    stupid_p2p_t *stupid_p2p = find_stupid_p2p(fd);
    if (stupid_p2p == NULL)
        return NULL;

    recv_data_buf_t *recv_data_buf = (recv_data_buf_t *)calloc(1, sizeof(recv_data_buf_t));
    if (recv_data_buf == NULL)
        return NULL;

    while (1) {
        recv_data_t *recv_data = recv_data_list_return_data(&stupid_p2p->recv_data_list);
        if (recv_data_num > max_num || recv_data == NULL)
            break;

        data_buf[recv_data_num++] = recv_data;
    }

    recv_data_buf->recv_data = (recv_data_t **)calloc(1, sizeof(recv_data_t **) * recv_data_num);
    if (recv_data_buf->recv_data == NULL) {
        free(recv_data_buf);
        for (int32_t i = 0; i < recv_data_num; i++)
            free(data_buf[i]);
        return NULL;
    }

    memcpy(recv_data_buf->recv_data, data_buf, sizeof(recv_data_t *) * recv_data_num);
    recv_data_buf->num = recv_data_num;

    return recv_data_buf;
}

void stupid_p2p_free_recv_data_buf(recv_data_buf_t *recv_data_buf)
{
    if (recv_data_buf == NULL)
        return ;

    for (int32_t i = 0; i < recv_data_buf->num; i++) {
        free(recv_data_buf->recv_data[i]->data);
        free(recv_data_buf->recv_data[i]);
    }

    free(recv_data_buf);
}