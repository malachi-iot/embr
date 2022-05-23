/**
 *  Inherently an SPI master thing, since SPI slave we are operating as (basically)
 *  just one raw device
 */
#pragma once

// esp-idf include
#include <driver/spi_master.h>

namespace embr { namespace esp_idf { namespace spi {

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

    inline operator spi_device_handle_t() const { return handle_; }
};

}}}
