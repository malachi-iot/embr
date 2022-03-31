// Adapted from
// https://github.com/OpenNuvoton/NUC472_442BSP/blob/master/SampleCode/FreeRTOS_lwIP_UDP_EchoServer/udp_echoserver-netconn.c

#include <embr/platform/lwip/netconn.h>
#include <cstring>

using namespace embr::lwip;

void _netconn_callback(struct netconn* conn, netconn_evt e, uint16_t len)
{

}

void udp_echo_init()
{
    Netconn c;
    Netbuf buf, buf_send;

    c.new_with_proto_and_callback(NETCONN_UDP, 0, _netconn_callback);

    err_t err;

    err = c.bind(NULLPTR, 7);

    if(err == ERR_OK)
    {
        for(;;)
        {
            err = c.recv(buf);

            const ip_addr_t* const addr = netbuf_fromaddr(buf.b());
            const uint16_t port = netbuf_fromport(buf.b());

            uint16_t len;
            void* payload_data;

            buf.data(&payload_data, &len);

            buf_send._new();
            void* data = buf_send.alloc(len);
            memcpy(data, payload_data, len);
            c.send(buf_send.b(), addr, port);

            buf_send.del();
            buf.del();
        }
    }

    c.del();
}