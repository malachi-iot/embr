#pragma once

#include <estd/internal/impl/streambuf.h>

#include "ipbuf.h"

#include "../tcp.h"

// In development, so experimental
namespace embr { namespace lwip { namespace experimental { 

enum tcp_pcb_ostreambuf_mode
{
    MODE_BUFFERED1,     // buffered via TCP_WRITE_FLAG_COPY copy
    MODE_BUFFERED2,     // buffered via our own managed buffer - has advantage of exposing pptr and friends
    MODE_UNBUFFERED     // zero-copy - BE CAREFUL!
};

template <class CharTraits, tcp_pcb_ostreambuf_mode = MODE_BUFFERED1,
    class Base = estd::internal::impl::streambuf_base<CharTraits> >
class tcp_pcb_ostreambuf : public Base
{
    using base_type = Base;
    
protected:
    typedef CharTraits traits_type;

    // DEBT: Change detail::streambuf to rely *only* on traits_type exposition
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

private:
    tcp::Pcb pcb_;

    // amount of buffer space left we can write to
    // DEBT: Look into formalizing xout_avail() or similar
    constexpr int_type xout_avail() const
    {
        return pcb_.sndbuf();
    }

    int_type sputc_ll(char_type c)
    {
        return pcb_.write(&c, 1, TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE) == 
            ERR_OK ? traits_type::to_int_type(c) : traits_type::eof();
    }

protected:
    int sync()
    {
        return pcb_.output() == ERR_OK ? 0 : -1;
    }

    int_type overflow(int_type ch = traits_type::eof())
    {
        if(pcb_.output() != ERR_OK) return traits_type::eof();
        if(traits_type::not_eof(ch) == false) return traits_type::eof() + 1;
        return sputc_ll(ch);
    }

public:
    tcp_pcb_ostreambuf() = default;
    tcp_pcb_ostreambuf(const tcp::Pcb& copy_from) :
        pcb_{copy_from}
    {}

    // https://lists.gnu.org/archive/html/lwip-users/2009-11/msg00018.html
    // PSH flag is probably ignored, but we'll be optimistic and say DON'T
    // push for individual characters
    int_type sputc(char c)
    {
        if(xout_avail() == 0)
            return overflow(c);
        else
            return sputc_ll(c);
    }

    int xsputn(const char* buf, int len)
    {
        // https://www.nongnu.org/lwip/2_1_x/group__tcp__raw.html#ga6b2aa0efbf10e254930332b7c89cd8c5
        // TODO: Heed xout_avail() and chunk it out
        // "The proper way to use this function is to call the function with at most tcp_sndbuf() bytes of data"

        pcb_.write(buf, len, TCP_WRITE_FLAG_COPY);
        return len;
    }
};

template <class CharTraits>
class tcp_pcb_istreambuf :
    //public estd::internal::impl::streambuf_base<CharTraits>
    public impl::ipbuf_streambuf<CharTraits>
{
    tcp::Pcb pcb_;


    void recv(tcp::Pcb tpcb, Pbuf p, err_t err)
    {

    }

    static void recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
    {
        ((tcp_pcb_istreambuf*)arg)->recv(tpcb, p, err);
    }

    void setup()
    {
        pcb_.arg(this);
        pcb_.recv(recv);
    }

public:
};

}}}