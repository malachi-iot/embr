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

public:
    typedef TCharTraits traits_type;
    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

    spi_master_ostreambuf(const spi::device& spi) : base_type(spi) {}
    spi_master_ostreambuf(spi::device&& spi) : base_type(std::move(spi)) {}

    // DEBT: This actually only works for int8_t/uint8_t compatible char_type.
    // Later on do some footwork to either expand that support or compiler-time
    // limit it
    int_type sputc(char_type c)
    {
        // Copy/pasted from [1] lcd_cmd
        esp_err_t ret;
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));       //Zero out the transaction
        t.length=8;                     //Command is 8 bits
        t.tx_buffer=&c;               //The data is the cmd itself
        t.user=(void*)0;                //D/C needs to be set to 0
        ret=spi_device_polling_transmit(spi, &t);  //Transmit!
        assert(ret==ESP_OK);            //Should have had no issues.
    }
};



}

template <class TChar, class TCharTraits = estd::char_traits<TChar> >
using basic_spi_master_ostreambuf = estd::internal::streambuf<impl::spi_master_ostreambuf<TCharTraits> >;

typedef basic_spi_master_ostreambuf<char> spi_master_ostreambuf;

}}