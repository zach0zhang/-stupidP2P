#include "stupidP2P.h"
#include "netWork.h"


#include <string.h>

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




static void to_hex(unsigned char *s, int l, char *d)
{
    const char hex_table[] = {
	'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	while (l--)
	{
		*(d++) = hex_table[*s >> 4];
		*(d++) = hex_table[*(s++) & 0x0F];
	}
    *(d++) = '\0';

}


void parse_recv_net_command(stupid_p2p_t *stupid_p2p)
{
    int32_t command_length = 0;
    uint8_t command[2048] = {0};

    uint8_t show_command[1024] = {0};


    if (stupid_p2p == NULL)
        return;

    while (recv_data_fifo_length(&stupid_p2p->recv_fifo) >= NET_PACKAGE_LENGTH) {
        command_length = recv_data_fifo_top_int(&stupid_p2p->recv_fifo);

        if (recv_data_fifo_length(&stupid_p2p->recv_fifo) >= (command_length + 4)) {
            recv_data_fifo_pop(&stupid_p2p->recv_fifo, sizeof(int32_t));
            for (int i = 0; i < command_length; i++) {
                command[i] = *(recv_data_fifo_top(&stupid_p2p->recv_fifo));
                recv_data_fifo_pop(&stupid_p2p->recv_fifo, 1);
            }

            //to_hex(command, command_length, show_command);
            //_log("%s\n", show_command);

            switch(command[0]) {
            case SEND_DATA: {
                uint8_t *data = (uint8_t *)calloc(1, sizeof(uint8_t) * (command_length - 1));
                if (data == NULL)
                    break;
                memcpy(data, &command[1], sizeof(uint8_t) * (command_length - 1));
                
                recv_data_t *recv_data = (recv_data_t *)calloc(1, sizeof(recv_data_t));
                if (recv_data == NULL) {
                    free(data);
                    break;
                }
                recv_data->data = data;
                recv_data->length = command_length - 1;

                
                recv_data_list_node_t *recv_data_node = (recv_data_list_node_t *)calloc(1, sizeof(recv_data_list_node_t));
                if (recv_data_node == NULL) {
                    free(data);
                    free(recv_data);
                    break;
                }

                recv_data_node->recv_data = recv_data;
                recv_data_list_add(&stupid_p2p->recv_data_list, recv_data_node);
                break;
            }
            case REGISTER_DEVICE_CALL_BACK:
                if (command_length != 2)
                    _log("register device callback length error\n");
                stupid_p2p->command_status.register_flag = (command[1] == EXEC_SUCCESS ? EXEC_SUCCESS : EXEC_FAIL);
                break;
            case SUBSCRIBE_DEVICE_CALL_BACK:
                if (command_length != 2)
                    _log("subscribe device callback length error\n");
                stupid_p2p->command_status.subscribe_flag = (command[1] == EXEC_SUCCESS ? EXEC_SUCCESS : EXEC_FAIL);
                break;
            case SEND_DATA_CALL_BACK:
                if (command_length != 2)
                    _log("send data callback length error\n");
                stupid_p2p->command_status.send_data_flag = (command[1] == EXEC_SUCCESS ? EXEC_SUCCESS : EXEC_FAIL);
                break;
            case UNREGISTER_DEVICE_CALL_BACK:
                if (command_length != 2)
                    _log("unregister device callback length error\n");
                stupid_p2p->command_status.unregister_flag = (command[1] == EXEC_SUCCESS ? EXEC_SUCCESS : EXEC_FAIL);
                break;
            case UNSUBSCRIBE_DEVICE_CALL_BACK:
                if (command_length != 2)
                    _log("unsubscribe device callback length error\n");
                stupid_p2p->command_status.unsubscribe_flag = (command[1] == EXEC_SUCCESS ? EXEC_SUCCESS : EXEC_FAIL);
                break;
            case CKECK_DEVICE_ALIVE_CALL_BACK:
                if (command_length != 2)
                    _log("check device alive callback length error\n");
                stupid_p2p->command_status.check_alive_flag = (command[1] == EXEC_SUCCESS ? EXEC_SUCCESS : EXEC_FAIL);
                break;
            case HEART_BEAT_CALL_BACK:
                if (command_length != 2)
                    _log("heart beat callback length error\n");
                stupid_p2p->command_status.check_alive_flag = (command[1] == EXEC_SUCCESS ? EXEC_SUCCESS : EXEC_FAIL);
                break;           
            default:
                _log("Invaild command\n");
                break;
            }

        } else
            return;
    }
}