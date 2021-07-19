#include "stupidP2P.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define DEVICE_ID "ABCD"
#define SUBSCRIBE_DEVICE_ID "FFSS"

typedef struct {
    uint8_t server_ip[64];
    int32_t server_port;

    uint8_t my_id[256];
    uint8_t other_id[256];
}client_t;

client_t g_client = {0};
pthread_t recv_id;

static void print_usage()
{
    printf("please input right params, like this:\n"); 
    printf("    ./p2pclient -s 182.160.5.196 -p 4040 -m client1 -o client2\n");
    printf("-s: p2p server address\n");
    printf("-p: p2p server port\n");
    printf("-m: my client id\n");
    printf("-o: connect target client id\n");
}

static int32_t parse_argv(int argc, char *argv[])
{
    int32_t ch;
    const char *optstring = "s:p:m:o:";
    while ((ch = getopt(argc, argv, optstring)) != -1) {
        switch (ch) {
        case 's':
            snprintf(g_client.server_ip, sizeof(g_client.server_ip), "%s", optarg);
            break;
        case 'p':
            g_client.server_port = atoi(optarg);
            break;
        case 'm':
            snprintf(g_client.my_id, sizeof(g_client.my_id), "%s", optarg);
            break;
        case 'o':
            snprintf(g_client.other_id, sizeof(g_client.other_id), "%s", optarg);
            break;
        case '?':
        default:
            print_usage();
            return -1;
        }
    }

    if (!strlen(g_client.server_ip) || !g_client.server_port || !strlen(g_client.my_id) || !strlen(g_client.other_id)) {
        print_usage();
        return -1;
    }

    return 0;
}

static void *_recv_func(void *argv)
{
    pthread_detach(pthread_self());

    int32_t *fd = (int32_t *)argv;
    int32_t ret;
    recv_data_buf_t *recv_data_buf = NULL;
    uint8_t data[2048] = {0};

    while (1) {
        recv_data_buf = stupid_p2p_recv_data(*fd);
        if (recv_data_buf != NULL) {
            for (int32_t i = 0; i < recv_data_buf->num; i++) {
                memcpy(data, recv_data_buf->recv_data[i]->data, recv_data_buf->recv_data[i]->length);
                data[recv_data_buf->recv_data[i]->length] = '\0';
                printf("recv data: %s\n", data);
            }
            stupid_p2p_free_recv_data_buf(recv_data_buf);
        }
        usleep(100 * 1000);
    }
}

int main(int argc, char *argv[])
{
    printf("pid: %d\n", getpid());

    int32_t ret = 0;
    int32_t fd = 0, loop = 0;
    uint8_t send_buf[2048] = {0};

    ret = parse_argv(argc, argv);
    if (ret)
        return ret;

    ret = stupid_p2p_init(g_client.server_ip, g_client.server_port);
    if (ret < 0)
        return ret;
    else
        fd = ret;

    ret = stupid_p2p_register_device(fd, g_client.my_id);
    if (ret < 0)
        goto end;
    else
        printf("register device:%s success \n", g_client.my_id);

    printf("checking device%s alive: \n", g_client.other_id);
    while (1) {
        sleep(1);
        ret = stupid_p2p_check_device_alive(fd, g_client.other_id);
        if (ret == STUPID_P2P_NO_ERROR) {
            printf("device:%s is alive\n", g_client.other_id);
            break;
        } else {
            printf("device:%s is not alive, try again %d\n",g_client.other_id, ++loop);
            if (loop >= 30)
                goto end;          
        }
    }


    ret = stupid_p2p_subscribe_device(fd, g_client.other_id);
    if (ret == STUPID_P2P_NO_ERROR)
        printf("subscribe device:%s success\n", g_client.other_id);
    else
        goto end;

    ret = pthread_create(&recv_id, NULL, _recv_func, (void *)&fd);
    if (ret != 0) {
        _log("%s: create receive thread error\n", __func__);
        goto end;
    }
    printf("input and receive(input 'quit' to quit): \n");    
    while(1) {
        fgets(send_buf, sizeof(send_buf), stdin);
        send_buf[strcspn(send_buf, "\n")] = '\0';
        if (strcmp(send_buf, "quit") == 0)
            break;
        if (strlen(send_buf)) {
            printf("send data: %s\n", send_buf);
            stupid_p2p_send_data(fd, send_buf, strlen(send_buf));
        }
    }
    


end:
    pthread_cancel(recv_id);
    pthread_join(recv_id, NULL);

    ret = stupid_p2p_unsubscribe_device(fd, g_client.other_id);
    if (ret == STUPID_P2P_NO_ERROR)
        printf("unsubscribe device:%s success\n", g_client.other_id);
    ret = stupid_p2p_unregister_device(fd);
    if (ret == STUPID_P2P_NO_ERROR)
        printf("unregister device%s success\n", g_client.my_id);
    
    stupid_p2p_deinit(fd);
}
