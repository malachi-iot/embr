#pragma once

#include <estd/internal/impl/streambuf.h>
#include <estd/streambuf.h>

#include "driver/spi_master.h"

namespace embr { namespace esp_idf {

namespace spi {

class device
{
    spi_device_handle_t handle_;

public:
    device(const device& copy_from) = default;
    device(device&& move_from)
    {
        handle_ = move_from.handle_;
        move_from.handle_ = nullptr;
    }
    device(const spi_host_device_t host_id, const spi_device_interface_config_t& dev_config)
    {
        ESP_ERROR_CHECK(spi_bus_add_device(host_id, &dev_config, &handle_));
    }

    ~device()
    {
        if(handle_ != nullptr)
            ESP_ERROR_CHECK(spi_bus_remove_device(handle_));
    }

    inline spi_device_handle_t handle() const { return handle_; }

    inline esp_err_t polling_start(spi_transaction_t* trans_desc, TickType_t ticks_to_wait)
    {
        return spi_device_polling_start(handle_, trans_desc, ticks_to_wait);
    }

    inline esp_err_t polling_end(TickType_t ticks_to_wait)
    {
        return spi_device_polling_end(handle_, ticks_to_wait);
    }

    inline esp_err_t polling_transmit(spi_transaction_t* trans_desc)
    {
        return spi_device_polling_transmit(handle_, trans_desc);
    }
};

}

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


template <class TCharTraits, spi_flags flags = spi_flags::Default>
class spi_master_ostreambuf :
    public spi_master_streambuf_base,
    public estd::internal::impl::streambuf_base<TCharTraits>
{
    typedef spi_master_streambuf_base base_type;

public:
    typedef TCharTraits traits_type;
    typedef typename traits_type::char_type char_type;

    spi_master_ostreambuf(const spi::device& spi) : base_type(spi) {}
    spi_master_ostreambuf(spi::device&& spi) : base_type(std::move(spi)) {}
};



}

template <class TChar, class TCharTraits = estd::char_traits<TChar> >
using basic_spi_master_ostreambuf = estd::internal::streambuf<impl::spi_master_ostreambuf<TCharTraits> >;

typedef basic_spi_master_ostreambuf<char> spi_master_ostreambuf;

}}