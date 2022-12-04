// Potential v2 of lwip transport.h

#include "embr/platform/lwip/features.h"
#include "embr/platform/lwip/endpoint.h"
#include "embr/platform/lwip/streambuf.h"
#include "embr/platform/lwip/pbuf.h"
#include "embr/platform/lwip/udp.h"

namespace embr { namespace experimental {


enum class transport_support
{
    preferred,
    native,
    emulated,
    unsupported
};

namespace tags {

template <transport_support s>
struct _support_level
{
    static constexpr transport_support support_level() { return s; }
};

struct read_blocking {};
struct read_callback {};
struct read_transaction {};

struct write_blocking {};
struct write_callback {};
struct write_transaction {};
struct write_fire_and_forget {};

}

template <class TNativeTransport>
struct transport_traits;


template <class TNativeBuffer>
struct buffer_traits;

enum class transport_results
{
    OK = 0    
};

template <>
struct transport_traits<udp_pcb> :
    tags::read_callback,
    tags::write_fire_and_forget
{
private:
    typedef udp_pcb native_type;
    typedef embr::lwip::udp::Pcb pcb_type;
    typedef pbuf* raw_pbuf;
    typedef const ip_addr_t* addr_pointer;
    typedef embr::lwip::PbufBase pbuf_type;

public:
    typedef pcb_type transport_type;
    typedef pbuf_type obuf_type;
    typedef pbuf_type ibuf_type;

    struct endpoint_type
    {
        addr_pointer addr;
        uint16_t port;
    };


    static void connect(pcb_type p, const endpoint_type& e)
    {
        p.connect(e.addr, e.port);
    }


    struct endpoint_transaction
    {
        endpoint_type endpoint;
        raw_pbuf buf;

        obuf_type out() const { return buf; }
        ibuf_type in() const { return buf; }

        bool write_completed() const { return buf == nullptr; }
        bool read_completed() const { return buf != nullptr; }
    };


    static transport_results begin_write(pcb_type pcb, endpoint_transaction* t, const endpoint_type* endpoint = nullptr)
    {
        embr::lwip::PbufBase buf(128);
        if(endpoint)
            t->endpoint = *endpoint;
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

    struct read_callback_type
    {
        pcb_type transport;
        ibuf_type in;
        endpoint_type endpoint;
    };

    template <typename F>
    static void recv_fn(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
    {
        F& f = *(F*) arg;

        endpoint_type e{addr, port};
        read_callback_type rct{pcb, p, e};

        f(rct);
    }


    template <typename F, typename TContext>
    static void recv_fn_no_closure(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
    {
        F& f = *(F*) nullptr; // DEBT: This will cause nasty problems if not used precisely right
        auto context = (TContext*) arg;

        endpoint_type e{addr, port};
        read_callback_type rct{pcb, p, e};

        f(rct, context);
    }


    template <typename F>
    inline static void read(pcb_type pcb, F&& f)
    {
        pcb.recv(recv_fn<F>, &f);
        //pcb.recv(&transport_traits::recv_fn, (void*)&f);
    }


    template <typename F, class TContext>
    inline static void read(pcb_type pcb, F&& f, TContext* context)
    {
        pcb.recv(recv_fn_no_closure<F>, context);
        //pcb.recv(&transport_traits::recv_fn, (void*)&f);
    }

    template <typename F>
    inline static void begin_read(pcb_type pcb, endpoint_transaction* t, F&& signal)
    {
        read(pcb, [=](const read_callback_type& rct)
        {
            t->endpoint = rct.endpoint;
            t->buf = rct.in;
            signal(rct);
        });
    }


    inline static void begin_read(pcb_type pcb, endpoint_transaction* t)
    {
        read(pcb, [=](const read_callback_type& rct)
        {
            t->endpoint = rct.endpoint;
            t->buf = rct.in;
        });
    }


    static void next_read(pcb_type pcb, endpoint_transaction* t)
    {

    }

    static void end_read(pcb_type pcb, endpoint_transaction* t)
    {

    }
};


template <>
struct buffer_traits<pbuf>
{

};


template <typename TNativeTransport, class TTraits = transport_traits<TNativeTransport> >
struct Transport;


template <class TTraits>
struct Transport<udp_pcb, TTraits>
{
    typedef TTraits traits_type;
    typedef typename traits_type::transport_type transport_type;

    transport_type native;

    typedef embr::lwip::opbuf_streambuf ostreambuf_type;
    typedef embr::lwip::ipbuf_streambuf istreambuf_type;

    Transport(transport_type native) : native{native} {}

    void begin_read() {}
    void end_read() {}
};


}}