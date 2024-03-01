#pragma once

#if ESP_PLATFORM
#include <esp_log.h>
#endif

#include "../netconn.h"
#include "../pbuf.h"


#include <estd/internal/impl/streambuf.h>

namespace embr { namespace lwip { namespace experimental {

class netconn_streambuf_untemplated
{
protected:
    Netconn conn_;

    // DEBT: For FreeRTOS event bit position only, and not relevant to all use cases
    // so optimize out via specialization or similar
    // NOTE: Hardcoded to one as we experiment, that will need to change
    const int event_id_ = 1;

    netconn_streambuf_untemplated(const Netconn& netconn) :
        conn_{netconn}
    {}

public:
    // EXPERIMENTAL, for traversal through singular callback.  Note also
    // if one copies the streambuf, multiples of these with the same ID are
    // going to appear.  Obviously public access is also a no no
    netconn_streambuf_untemplated* next_ = nullptr;

    constexpr int event_id() const
    {
        return event_id_;
    }

    constexpr bool is_match(netconn* n) const
    {
        return conn_.native() == n;
    }
};


template <class CharTraits,
    class Base = estd::internal::impl::streambuf_base<CharTraits> >
class netconn_streambuf_base : public Base,
    public netconn_streambuf_untemplated
{
protected:
    netconn_streambuf_base(const Netconn& netconn) :
        netconn_streambuf_untemplated{netconn}
    {}

public:
};

template <class CharTraits,
    class Base = netconn_streambuf_base<CharTraits> >
class netconn_ostreambuf : public Base
{
    using base_type = Base;

    // DEBT: Do a bipbuffer instead.  This ought to get us going, though we'll get a
    // crush factor when high speed sends overwhelm sync() creating less and less
    // full writes as you close to the buffer end
    v1::PbufBase out_;

    using size_type = estd::streamsize;
    uint16_t pos_begin_ = 0, pos_end_ = 0;
    // DEBT: Thunking means this is the max we'll ever emit per thunk.
    // Will want to explore 'vector' flavor for a pseudo-queue/scatter gather
    // approach
    static constexpr unsigned tot_len_ = 128;

protected:
    constexpr uint16_t to_send() const
    {
        return pos_end_ - pos_begin_;
    }

    int sync(uint8_t flags = NETCONN_DONTBLOCK)
    {
        // All sync'd up, leave
        if(pos_begin_ == pos_end_) return 0;

        // TODO: Deep dive into write_partly to see what the nature of
        // the DONTBLOCK buffer treatment really is (does it ever get copied?
        // how do we know it's safe to release our own buffer?)
        // 10 minute dive tells us "netconn_apimsg" is called with a semaphore which
        // appears to wait until the tcp thread has picked up and completed
        // "lwip_netconn_do_write" and "lwip_netconn_do_writemore".
        // These do appear to sequentially update bytes_written via results of
        // "tcp_write" (our buddy!)
        size_t bytes_written;
        err_t r = base_type::conn_.write_partly(
            pbase() + pos_begin_, to_send(),
            flags,
            &bytes_written
            );

        pos_begin_ += bytes_written;

        if(pos_begin_ == pos_end_)
            pos_begin_ = pos_end_ = 0;

        return r == ERR_OK ? 0 : -1;
    }

    constexpr uint16_t xout_avail() const
    {
        return tot_len_ - pos_end_;
    }

public:
    using typename Base::char_type;
    using typename Base::int_type;
    using typename Base::traits_type;

    //char_type* eback() const
    char_type* pbase() const
    {
        return static_cast<char_type*>(out_.payload());
    }

    char_type* pptr() const
    {
        return pbase() + pos_end_;
    }

    char_type* epptr() const
    {
        // NOTE: Docs indicate pbuf won't be chained (which is a bit of a bummer and
        // revelation) - but for the time being that's convenient, this length() will
        // be same as tot_length
        return pbase() + out_.length();
    }

    netconn_ostreambuf(const Netconn& conn) :
        base_type(conn),
        out_(tot_len_, PBUF_RAW) {}

    int_type sputc(char_type c)
    {
        sync(NETCONN_DONTBLOCK | NETCONN_MORE);
        *pptr() = c;
        ++pos_end_;
        sync(NETCONN_DONTBLOCK | NETCONN_MORE);
        return c;
    }

    size_type xsputn(const char_type* s, size_type count)
    {
        sync(NETCONN_DONTBLOCK | NETCONN_MORE);

        uint16_t to_write = xout_avail();

        if(count < to_write)    to_write = count;

        memcpy(pptr(), s, to_write);

        pos_end_ += to_write;

        if(to_write == count)
            sync();
        else
            sync(NETCONN_DONTBLOCK | NETCONN_MORE);

        return to_write;
    }
};


template <class CharTraits,
    class Base = netconn_streambuf_base<CharTraits> >
class netconn_istreambuf : public Base
{
    using base_type = Base;

#if ESP_PLATFORM
    static constexpr const char* TAG = "netconn_istreambuf";
#endif

protected:
    // DEBT: Whipping this up, but really we need proper pbuf_istreambuf as a base class here
    PbufBase in_;
    uint16_t pos_ = 0;

    constexpr uint16_t xin_avail() const
    {
        if(!in_.valid()) return 0;

        return in_.length() - pos_;
    }

    err_t sync_ll()
    {
        pbuf* new_buf;

        err_t r = base_type::conn_.recv_tcp_pbuf(
            &new_buf,
            NETCONN_DONTBLOCK);

#if ESP_PLATFORM
        ESP_LOGV(TAG, "sync_ll c=%c", *(char*)new_buf->payload);
#endif
        in_ = new_buf;

        return r;
    }

    int sync()
    {
        // DEBT: Crude way to handle start condition before we receive anything.  I think
        // i'd prefer some kind of ROM 0-buf
        if(in_.valid())
        {
            // DEBT: Can't sync until all data has been read.  We'll need a proper packet
            // queue for that (and probably for ostreambuf too)
            if(pos_ != in_.length()) return -1;

            if(pos_ > in_.length())
            {
                assert("Looks like chain DEBT caught up to us");
            }

            // We finally consumed all of the pbuf
            in_.free();
            pos_ = 0;
        }

        return sync_ll() == ERR_OK ? 0 : -1;
    }

public:
    using typename Base::char_type;
    using typename Base::int_type;
    using typename Base::traits_type;

    char_type* eback() const
    {
        return static_cast<char_type*>(in_.payload());
    }

    char_type* gptr() const
    {
        return eback() + pos_;
    }

    char_type* egptr() const
    {
        return eback() + in_.length();
    }

    void gbump(int count)
    {
        pos_ += count;
    }

    estd::streamsize showmanyc()
    {
        return xin_avail();
    }

    int_type underflow()
    {
        sync();
        if(xin_avail() == 0) return traits_type::eof();
        return traits_type::to_int_type(*gptr());
    }

    char_type xsgetc() const { return *gptr(); }

    estd::streamsize xsgetn(char_type* dest, estd::streamsize count)
    {
        sync();

        // FIX: Chaining could goof us up here. Watch out
        uint16_t copied = in_.copy_partial(dest, count, pos_);

        pos_ += copied;

        sync();

        return copied;
    }

    //ESTD_CPP_FORWARDING_CTOR(netconn_istreambuf)
    netconn_istreambuf(const Netconn& conn) :
        base_type(conn),
        in_(nullptr)    // DEBT
    {}
};


}}}
