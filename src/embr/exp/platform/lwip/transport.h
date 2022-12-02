// Potential v2 of lwip transport.h

#include "embr/platform/lwip/features.h"
#include "embr/platform/lwip/endpoint.h"
#include "embr/platform/lwip/streambuf.h"
#include "embr/platform/lwip/pbuf.h"
#include "embr/platform/lwip/udp.h"

namespace embr { namespace experimental {

template <class TNativeTransport>
struct transport_traits;

enum class transport_results
{
    OK = 0    
};

template <>
struct transport_traits<udp_pcb>
{
    typedef udp_pcb native_type;

private:
    typedef embr::lwip::udp::Pcb pcb_type;
    typedef pbuf* raw_pbuf;
    typedef const ip_addr_t* addr_pointer;
    typedef embr::lwip::PbufBase pbuf_type;

public:
    typedef pbuf_type obuf_type;
    typedef pbuf_type ibuf_type;

    struct endpoint_type
    {
        addr_pointer addr;
        uint16_t port;
    };


    struct endpoint_transaction
    {
        const endpoint_type* endpoint;
        raw_pbuf buf;

        obuf_type out() const { return buf; }
        bool completed() const { return buf == nullptr; }
    };


    static transport_results begin_write(pcb_type pcb, endpoint_transaction* t, const endpoint_type* endpoint = nullptr)
    {
        embr::lwip::PbufBase buf(128);
        t->endpoint = endpoint;
        t->buf = buf;
        return transport_results::OK;
    }


    static void end_write(pcb_type pcb, endpoint_transaction* t)
    {
        pcb.send(t->buf);
        pbuf_free(t->buf);
        t->buf = nullptr;
    }

    static void write(native_type* pcb, obuf_type buffer)
    {

    }

    static void write(native_type* pcb, obuf_type buffer, endpoint_type e)
    {

    }
};

}}