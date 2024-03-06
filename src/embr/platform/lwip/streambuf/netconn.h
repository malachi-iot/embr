#pragma once

#if ESP_PLATFORM
#include <esp_log.h>
#endif

#include "../netconn.h"
#include "../pbuf.h"


#include <estd/internal/impl/streambuf.h>

#if FEATURE_ESTD_STREAMBUF_TRAITS
#include <estd/port/freertos/event_groups.h>
#endif

#ifndef FEATURE_EMBR_LWIP_NETCONN_EVENT
#define FEATURE_EMBR_LWIP_NETCONN_EVENT 1
#endif


namespace embr { namespace lwip { namespace experimental {

class netconn_streambuf_untemplated
{
    using this_type = netconn_streambuf_untemplated;

protected:
    Netconn conn_;

#if FEATURE_EMBR_LWIP_NETCONN_EVENT
    // DEBT: For FreeRTOS event bit position only, and not relevant to all use cases
    // so optimize out via specialization or similar
    // NOTE: Hardcoded to one as we experiment, that will need to change
    const int event_id_ = 1;
#endif

    netconn_streambuf_untemplated(const Netconn& netconn) :
        conn_{netconn}
    {}

public:
#if FEATURE_EMBR_LWIP_NETCONN_EVENT
    // EXPERIMENTAL, for traversal through singular callback.  Note also
    // if one copies the streambuf, multiples of these with the same ID are
    // going to appear.  Obviously public access is also a no no
    netconn_streambuf_untemplated* next_ = nullptr;

    constexpr int event_id() const
    {
        return event_id_;
    }
#endif

    constexpr bool is_match(netconn* n) const
    {
        return conn_.native() == n;
    }

#if FEATURE_ESTD_STREAMBUF_TRAITS
    // Watch out for naming here, this traits_type isn't usable on its own, instead MI
    // must be used later down the line
    struct traits_type
    {
        struct signal
        {
            static constexpr int cts_bit_ = 1;
            static constexpr int dtr_bit_ = 2;
            static constexpr estd::chrono::milliseconds timeout { 100 };

            estd::freertos::event_group<true> event_;

            // TODO: Feature flag in RTOS mode specifically, otherwise you might have to
            // poll it anyway
            bool wait_dtr(this_type* sb)
            {
                return event_.wait_bits(dtr_bit_, true, true,
                    timeout) & dtr_bit_;
            }

            // Auto subtract len_exp from awaiting_ack_bytes_ (EXPERIMENTAL)
            // on success
            bool wait_cts(this_type* sb, int len_exp = 0)
            {
                bool r = event_.wait_bits(cts_bit_, true, true,
                    timeout) & cts_bit_;

                // Happening in event handler proper, don't do here
                //if(r)   sb->awaiting_ack_bytes_ -= len_exp;

                return r;
            }

            void set_dtr()
            {
                event_.set_bits(dtr_bit_);
            }

            void set_cts()
            {
                event_.set_bits(cts_bit_);
            }
        };
    };

    // accumulate how many bytes we sent off, idea being we decrement them (perhaps manually)
    // on consuming during nocopy, perhaps during a special wait_cts which takes a length param?
    unsigned awaiting_ack_bytes_ = 0;

    // DEBT: For now no extra linked list trickery, 1 streambuf is associated to 1 at most
    // ostream/istream
    traits_type::signal* signal_ = nullptr;
#endif
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
#if FEATURE_ESTD_STREAMBUF_TRAITS
    struct traits_type : CharTraits,
        netconn_streambuf_untemplated::traits_type
    {

    };

    void signal(traits_type::signal* v)
    {
        signal_ = v;
    }
#endif
};

class netconn_ostreambuf_untemplated
{
protected:
    // DEBT: Do a bipbuffer instead.  This ought to get us going, though we'll get a
    // crush factor when high speed sends overwhelm sync() creating less and less
    // full writes as you close to the buffer end
    v1::PbufBase out_;

    using size_type = estd::streamsize;
    // begin = where to start next send from
    // end = current position for consumer + where to end next send
    uint16_t pos_begin_ = 0, pos_end_ = 0;
    // NETCONN_EVT_SENDPLUS *probably* indicates how to update this
    // https://doc.ecoscentric.com/ref/lwip-api-sequential-netconn-new-with-callback.html
    uint16_t pos_end_ack_ = 0;
    // DEBT: Thunking means this is the max we'll ever emit per thunk.
    // Will want to explore 'vector' flavor for a pseudo-queue/scatter gather
    // approach
    static constexpr unsigned tot_len_ = 128;

    ESTD_CPP_FORWARDING_CTOR_MEMBER(netconn_ostreambuf_untemplated, out_)
};

template <class CharTraits,
    class Base = estd::internal::impl::streambuf_base<CharTraits> >
class netconn_copy_ostreambuf : public Base
{
    using base_type = Base;

    union
    {
        unsigned raw_ = 0;

        struct
        {
            bool nocopy_ : 1;
        };
    };

protected:
    Netconn conn_;

    // EXPERIMENTAL, pretty sure won't work.  Trying to force an immediate PSH
    int sync()
    {
        size_t bytes_written;

        err_t r = conn_.write_partly(nullptr, 0,
            NETCONN_DONTBLOCK,
            &bytes_written);

        if(r != ERR_OK) return traits_type::eof();

    }

public:
    using typename Base::char_type;
    using typename Base::int_type;
    using typename Base::traits_type;
    using size_type = estd::streamsize;

    int_type sputc(char_type c)
    {
        size_t bytes_written;

        err_t r = conn_.write_partly(&c, 1,
            NETCONN_DONTBLOCK | NETCONN_COPY | NETCONN_MORE,
            &bytes_written);

        if(r != ERR_OK) return traits_type::eof();

        return traits_type::to_int_type(c);
    }

    size_type xsputn(const char_type* s, size_type count)
    {
        size_t bytes_written;

        err_t r = conn_.write_partly(s, count,
            NETCONN_DONTBLOCK | NETCONN_COPY | NETCONN_MORE,
            &bytes_written);

        if(r != ERR_OK) return 0;

        return bytes_written;
    }

    netconn_copy_ostreambuf(const Netconn& conn) :
        conn_{conn}
    {}
};

// DEBT: 'nocopy' is a slight misnomer.  While true, the characteristic
// is specifically the presence of an intermediate buffer to which
// external parties may interact directly and which we lift from without
// copying.  Accurate.  However, the 'copy' variety technically could
// do some nocopy too if you can assure it that the outgoing buffer is
// non volatile.
template <class CharTraits,
    class Base = netconn_streambuf_base<CharTraits> >
class netconn_nocopy_ostreambuf : public Base,
    public netconn_ostreambuf_untemplated
{
    using base_type = Base;

    // When true, indicates that end of an xsputn phase should send a PSH
    // once it fully writes out the data.  I speculate setting this to false
    // may increase bandwidth somewhat, while true decreases latency
    constexpr bool do_psh()
    {
        return true;
    }

    // When true, leverages nocopy on xsputn.  Buffers cannot change until
    // ACK is received!
    constexpr bool ro_mode()
    {
        return false;
    }

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
        // "tcp_write" (our buddy!).  Doc from "tcp_write" tells us:
        // "This also means that the memory behind dataptr must not change until the data is ACKed by the remote host"
        size_t bytes_written;
        err_t r = base_type::conn_.write_partly(
            pbase() + pos_begin_, to_send(),
            flags,
            &bytes_written
            );

        pos_begin_ += bytes_written;

#if FEATURE_ESTD_STREAMBUF_TRAITS
        base_type::awaiting_ack_bytes_ += bytes_written;
#endif

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

    netconn_nocopy_ostreambuf(const Netconn& conn) :
        base_type(conn),
        netconn_ostreambuf_untemplated(tot_len_, PBUF_RAW) {}

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

        uint16_t to_write;
        size_t bytes_written;

        if(ro_mode() && to_send() == 0)
        {
            err_t r = base_type::conn_.write_partly(
                s, count,
                NETCONN_DONTBLOCK | NETCONN_MORE,
                &bytes_written);

            if(r != ERR_OK) return 0;

#if FEATURE_ESTD_STREAMBUF_TRAITS
            base_type::awaiting_ack_bytes_ += bytes_written;
#endif

            // If write succeeded and *ALL* bytes written, then no need
            // for local buffer shenanigans - we're out
            if(bytes_written == count) return count;

            // Otherwise, skip by what was written and cache up the rest
            s += bytes_written;
            to_write = count - bytes_written;
        }
        else
        {
            bytes_written = 0;
            to_write = count;
        }

        if (to_write > xout_avail())
            to_write = xout_avail();

        memcpy(pptr(), s, to_write);

        pos_end_ += to_write;

        if(do_psh() && bytes_written + to_write == count)
            // If we definitely write 100% of what was asked of us,
            // flag PSH
            sync();
        else
            sync(NETCONN_DONTBLOCK | NETCONN_MORE);

        return bytes_written + to_write;
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
