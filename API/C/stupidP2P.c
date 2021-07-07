#include "stupidP2P.h"

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct stupid_p2p_client_node {
    int32_t fd;
    int32_t socket_fd;
    struct sockaddr_in server_addr;
    pthread_t stupid_client_receive_id;
    pthread_t stupid_client_send_id;
    struct stupid_p2p_client_node *next;
};

typedef struct stupid_p2p_client_node stupid_p2p_t;
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


static void *_stupid_p2p_recv_func(void *argv)
{
    pthread_detach(pthread_self());
    stupid_p2p_t *stupid_p2p = (stupid_p2p_t *)argv;
    int32_t ret;
    int8_t buf[1024] = {0};

    while (1) {
        ret = recv(stupid_p2p->socket_fd, buf, sizeof(buf), 0);
        _log("receive %d bytes: %s\n", ret, buf);
    }

    return NULL;
}

static void *_stupid_p2p_send_func(void *argv)
{
    pthread_detach(pthread_self());
    stupid_p2p_t *stupid_p2p = (stupid_p2p_t *)argv;

    while(1) {
        sleep(1);
    }

    return NULL;
}

int32_t stupid_p2p_init(const char *server_ip, const int32_t server_port)
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

    return add_stupid_p2p_to_list(stupid_p2p);
}

void stupid_p2p_deinit(int32_t fd)
{
    stupid_p2p_t *stupid_p2p = find_stupid_p2p(fd);
    if (stupid_p2p == NULL)
        return;

    pthread_cancel(stupid_p2p->stupid_client_receive_id);
    pthread_cancel(stupid_p2p->stupid_client_send_id);
    pthread_join(stupid_p2p->stupid_client_receive_id, NULL);
    pthread_join(stupid_p2p->stupid_client_send_id, NULL);
    
    close(stupid_p2p->socket_fd);
    remove_stupid_p2p(stupid_p2p->fd);

}