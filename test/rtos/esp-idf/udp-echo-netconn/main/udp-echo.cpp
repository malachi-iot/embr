// Adapted from
// https://github.com/OpenNuvoton/NUC472_442BSP/blob/master/SampleCode/FreeRTOS_lwIP_UDP_EchoServer/udp_echoserver-netconn.c

#include <embr/platform/lwip/netconn.h>
#include <cstring>

using namespace embr::lwip;

void _netconn_callback(struct netconn* conn, netconn_evt e, uint16_t len)
{
    switch(e)
    {
        case NETCONN_EVT_RCVPLUS:
            break;

        case NETCONN_EVT_RCVMINUS:
            break;

        case NETCONN_EVT_SENDPLUS:
            break;

        case NETCONN_EVT_SENDMINUS:
            break;

        case NETCONN_EVT_ERROR:
            break;
    }
}

void udp_echo_init()
{
    experimental::AutoNetconn c(NETCONN_UDP, 0, _netconn_callback);

    err_t err;

    err = c.bind(7);

    if(err == ERR_OK)
    {
        for(;;)
        {
            Netbuf buf;
            err = c.recv(&buf);

            Endpoint e = buf.fromendpoint();

            uint16_t len;
            void* payload_data;

            buf.data(&payload_data, &len);

            experimental::AutoNetbuf buf_send;

            void* data = buf_send.alloc(len);
            memcpy(data, payload_data, len);
            c.send(buf_send, e);

            buf.del();
        }
    }
}