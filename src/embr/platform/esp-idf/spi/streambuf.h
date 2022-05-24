/**
 *
 * References:
 *
 * 1. https://github.com/espressif/esp-idf/blob/v4.4.1/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c
 */
#pragma once

#include <estd/queue.h>
#include <estd/internal/impl/streambuf.h>
#include <estd/streambuf.h>

#include "device.h"

// esp-idf include
#include <esp_log.h>
#include <driver/spi_master.h>

namespace embr { namespace esp_idf {

namespace impl {

enum class spi_flags : uint16_t
{
    StandardBuffer = 1,
    DMA = 2,

    MemtypeMask = 3,

    Polled = 4,
    Interrupt = 8,

    TechniqueMask = 12,

    Buffered = 16,

    Default = spi_flags::StandardBuffer | spi_flags::Polled
};

class spi_master_streambuf_base
{
protected:
    spi::device spi;

    spi_master_streambuf_base(const spi::device& spi) : spi(spi) {}
    spi_master_streambuf_base(spi::device&& spi) : spi(std::move(spi)) {}
};

template <class TCharTraits, spi_flags flags = spi_flags::Default>
class spi_master_ostreambuf;

// Starting out as blocking polled version, to make life easy
// Be careful because estd really expects streambufs to be nonblocking
template <class TCharTraits>
class spi_master_ostreambuf<TCharTraits, spi_flags::Default> :
    public spi_master_streambuf_base,
    public estd::internal::impl::streambuf_base<TCharTraits>
{
    constexpr static char* TAG = "spi_master_ostreambuf";

    typedef spi_master_streambuf_base base_type;

    spi_transaction_t t;
    // DEBT: Optimize this
    void* user_;

public:
    typedef TCharTraits traits_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    spi_master_ostreambuf(const spi::device& spi) : base_type(spi) {}
    spi_master_ostreambuf(spi::device&& spi) : base_type(std::move(spi)) {}

    void user(void* v) { user_ = v; }

    // DEBT: This actually only works for int8_t/uint8_t compatible char_type.
    // Later on do some footwork to either expand that support or compiler-time
    // limit it
    int_type sputc(char_type c)
    {
        // Copy/pasted from [1] lcd_cmd
        esp_err_t ret;
        memset(&t, 0, sizeof(t));       //Zero out the transaction
        t.length=8;                     // Hard-wired to 8 bits
        t.tx_buffer=&c;
        t.user=user_;
        ret=spi.polling_transmit(&t);  //Transmit!
        //ESP_LOGD(TAG, "sputc: %d characters sent", t.length);
        if(ret != ESP_OK)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
            return traits_type::eof();
        }
        return traits_type::to_int_type(c);
    }

    estd::streamsize xsputn(const char_type* s, estd::streamsize count)
    {
        // Copy/pasted from [1] lcd_data
        esp_err_t ret;
        if (count==0) return 0;             //no need to send anything
        memset(&t, 0, sizeof(t));       //Zero out the transaction
        t.length=count*8;                 //Len is in bytes, transaction length is in bits.
        t.tx_buffer=s;               //Data
        t.user=user_;
        ret=spi.polling_transmit(&t);  //Transmit!
        if(ret != ESP_OK)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
            count = 0;
        }
        return count;
    }
};

// DEBT: The polled/blocking flavor we can combine more or less
template <class TCharTraits, spi_flags flags = spi_flags::Default>
class spi_master_istreambuf : public spi_master_streambuf_base,
    public estd::internal::impl::streambuf_base<TCharTraits>,
    public estd::internal::streambuf_sbumpc_tag
{
    constexpr static char* TAG = "spi_master_istreambuf";

    typedef spi_master_streambuf_base base_type;

    spi_transaction_t t;
    // DEBT: Optimize this
    void* user_;

public:
    typedef TCharTraits traits_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    constexpr static unsigned char_bitcount = 8 * sizeof(char_type);

    spi_master_istreambuf(const spi::device& spi) : base_type(spi) {}
    spi_master_istreambuf(spi::device&& spi) : base_type(std::move(spi)) {}

    int_type sbumpc()
    {
        memset(&t, 0, sizeof(t));
        t.user = user_;
        t.length=char_bitcount;
        t.flags = SPI_TRANS_USE_RXDATA;

        esp_err_t ret = spi.polling_transmit(&t);

        ESP_LOGV(TAG, "sbumpc: %d characters received", t.rxlength / char_bitcount);

        if(ret != ESP_OK)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
            return traits_type::eof();
        }
        if(sizeof(char_type) == 1)
            return traits_type::to_int_type(t.rx_data[0]);
        else
            return traits_type::to_int_type(*(char_type*)t.rx_data);
    }

    void user(void* v) { user_ = v; }

    estd::streamsize xsgetn(char_type* s, estd::streamsize count)
    {
        memset(&t, 0, sizeof(t));
        t.length=8*count;
        t.user = user_;

        /*
         * This is nifty, but the way we do things in xsgetn it's doubtful it will ever be more efficient
        if(count <= 4)
        {
            t.flags = SPI_TRANS_USE_RXDATA;

            esp_err_t ret = spi.polling_transmit(&t);
            //assert( ret == ESP_OK );
            if(ret != ESP_OK)
            {
                ESP_LOGD(TAG, "xsgetn encountered bus error");
                return traits_type::eof();
            }
            memcpy(s, t.rx_data, count);
            return count;
        }
        else */
        {
            t.rx_buffer = s;

            esp_err_t ret = spi.polling_transmit(&t);
            //assert( ret == ESP_OK );
            if(ret != ESP_OK)
            {
                ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
                return traits_type::eof();
            }
            return count;
        }
    }
};


// Unbuffered
template <class TCharTraits>
class spi_master_ostreambuf<TCharTraits, spi_flags::Interrupt> :
    public spi_master_streambuf_base,
    public estd::internal::impl::streambuf_base<TCharTraits>
{
    constexpr static char* TAG = "spi_master_ostreambuf";

    typedef spi_master_streambuf_base base_type;

    estd::layer1::deque<spi_transaction_t, 6> queue;
    // DEBT: Optimize this
    void* user_;

public:
    typedef TCharTraits traits_type;
    using char_type = typename traits_type::char_type;
    using int_type = typename traits_type::int_type;

    constexpr spi_master_ostreambuf(const spi::device& spi) : base_type(spi) {}

    void user(void* v) { user_ = v; }

    int_type sputc(char_type c)
    {
        esp_err_t ret;

        // Make sure we don't roll over
        if(queue.size() == queue.max_size())
            return traits_type::eof();

        spi_transaction_t& t = queue.emplace_back();

        memset(&t, 0, sizeof(t));       //Zero out the transaction
        t.flags = SPI_TRANS_USE_TXDATA;
        t.length=8;                     // Hard-wired to 8 bits
        t.tx_data[0] = c;
        t.user=user_;

        // Non blocking variety
        ret=spi.queue_trans(&t, 0);  //Transmit!
        if(ret != ESP_OK)
        {
            queue.pop_back();
            ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
            return traits_type::eof();
        }
        return traits_type::to_int_type(c);
    }

    estd::streamsize xsputn(const char_type* s, estd::streamsize count)
    {
        esp_err_t ret;

        // Make sure we don't roll over
        if(queue.size() == queue.max_size())
            return traits_type::eof();

        spi_transaction_t& t = queue.emplace_back();
        if (count==0) return 0;             //no need to send anything
        memset(&t, 0, sizeof(t));       //Zero out the transaction
        t.length=count*8;                 //Len is in bytes, transaction length is in bits.
        t.tx_buffer=s;               //Data
        t.user=user_;
        ret=spi.queue_trans(&t, 0);  //Transmit nonblocking
        if(ret != ESP_OK)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
            count = 0;
        }
        return count;
    }

};

}

template <class TChar, class TCharTraits = estd::char_traits<TChar> >
using basic_spi_master_ostreambuf = estd::internal::streambuf<impl::spi_master_ostreambuf<TCharTraits> >;

typedef basic_spi_master_ostreambuf<char> spi_master_ostreambuf;

template <class TChar, class TCharTraits = estd::char_traits<TChar> >
using basic_spi_master_istreambuf = estd::internal::streambuf<impl::spi_master_istreambuf<TCharTraits> >;

typedef basic_spi_master_istreambuf<char> spi_master_istreambuf;

template <class TChar, class TCharTraits = estd::char_traits<TChar> >
using basic_test_interrupt_ostreambuf = estd::internal::streambuf<impl::spi_master_ostreambuf<TCharTraits, impl::spi_flags::Interrupt> >;

typedef basic_test_interrupt_ostreambuf<char> test_interrupt_ostreambuf;

}}