/**
 *
 * References:
 *
 * 1. https://github.com/espressif/esp-idf/blob/v4.4.1/examples/peripherals/spi_master/lcd/main/spi_master_example_main.c
 */
#pragma once

#include <estd/internal/impl/streambuf.h>
#include <estd/streambuf.h>

#include "device.h"

// esp-idf include
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

    Default = spi_flags::StandardBuffer | spi_flags::Polled
};

class spi_master_streambuf_base
{
protected:
    spi::device spi;

    spi_master_streambuf_base(const spi::device& spi) : spi(spi) {}
    spi_master_streambuf_base(spi::device&& spi) : spi(std::move(spi)) {}
};


// Starting out as blocking polled version, to make life easy
// Be careful because estd really expects streambufs to be nonblocking
template <class TCharTraits, spi_flags flags = spi_flags::Default>
class spi_master_ostreambuf :
    public spi_master_streambuf_base,
    public estd::internal::impl::streambuf_base<TCharTraits>
{
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
        ret=spi_device_polling_transmit(spi, &t);  //Transmit!
        assert(ret==ESP_OK);            //Should have had no issues.
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
        ret=spi_device_polling_transmit(spi, &t);  //Transmit!
        assert(ret==ESP_OK);            //Should have had no issues.
        return count;
    }
};

// DEBT: The polled/blocking flavor we can combine more or less
template <class TCharTraits, spi_flags flags = spi_flags::Default>
class spi_master_istreambuf : public spi_master_streambuf_base,
    public estd::internal::impl::streambuf_base<TCharTraits>,
    public estd::internal::streambuf_sbumpc_tag
{
    typedef spi_master_streambuf_base base_type;

    spi_transaction_t t;
    // DEBT: Optimize this
    void* user_;

public:
    typedef TCharTraits traits_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    spi_master_istreambuf(const spi::device& spi) : base_type(spi) {}
    spi_master_istreambuf(spi::device&& spi) : base_type(std::move(spi)) {}

    int_type sbumpc()
    {
        /*
        memset(&t, 0, sizeof(t));
        t.length=8*3;
        t.flags = SPI_TRANS_USE_RXDATA;
        t.user = (void*)1;

        esp_err_t ret = spi_device_polling_transmit(spi, &t);
        assert( ret == ESP_OK );

        return *(uint32_t*)t.rx_data; */
        return traits_type::eof();
    }

    void user(void* v) { user_ = v; }

    estd::streamsize xsgetn(char_type* s, estd::streamsize count)
    {
        if(count <= 4)
        {
            memset(&t, 0, sizeof(t));
            t.length=8*count;
            t.flags = SPI_TRANS_USE_RXDATA;
            t.user = user_;

            esp_err_t ret = spi_device_polling_transmit(spi, &t);
            assert( ret == ESP_OK );
            return count;
        }
        else
        {
            return -1;
        }
    }
};


}

template <class TChar, class TCharTraits = estd::char_traits<TChar> >
using basic_spi_master_ostreambuf = estd::internal::streambuf<impl::spi_master_ostreambuf<TCharTraits> >;

typedef basic_spi_master_ostreambuf<char> spi_master_ostreambuf;

template <class TChar, class TCharTraits = estd::char_traits<TChar> >
using basic_spi_master_istreambuf = estd::internal::streambuf<impl::spi_master_istreambuf<TCharTraits> >;

typedef basic_spi_master_istreambuf<char> spi_master_istreambuf;

}}