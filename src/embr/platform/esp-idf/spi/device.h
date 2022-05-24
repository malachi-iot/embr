/**
 *  Inherently an SPI master thing, since SPI slave we are operating as (basically)
 *  just one raw device
 */
#pragma once

// esp-idf include
#include <esp_log.h>
#include <driver/spi_master.h>

#include "transaction.h"

namespace embr { namespace esp_idf { namespace spi {

class device_base
{
protected:
    //constexpr static const char* TAG = "spi::device";

    spi_device_handle_t handle_;

public:
    //inline spi_device_handle_t handle() const { return handle_; }

    inline esp_err_t queue_trans(spi_transaction_t* trans_desc, TickType_t ticks_to_wait)
    {
        return spi_device_queue_trans(handle_, trans_desc, ticks_to_wait);
    }

    inline esp_err_t get_trans_result(spi_transaction_t** trans_desc, TickType_t ticks_to_wait)
    {
        return spi_device_get_trans_result(handle_, trans_desc, ticks_to_wait);
    }

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

    inline esp_err_t add(const spi_host_device_t host_id, const spi_device_interface_config_t& dev_config)
    {
        return spi_bus_add_device(host_id, &dev_config, &handle_);
    }

    inline esp_err_t remove()
    {
        return spi_bus_remove_device(handle_);
    }
};

struct device : device_base
{
    device() = default;
    inline device(const device& copy_from) = default;
    inline device(spi_device_handle_t handle)
    {
        handle_ = handle;
    }

    device& operator=(const device& copy_from) = default;
};

struct managed_device : device_base
{
    managed_device(const managed_device& copy_from) = default;
    managed_device(managed_device&& move_from)
    {
        handle_ = move_from.handle_;
        move_from.handle_ = nullptr;
    }
    managed_device(const spi_host_device_t host_id, const spi_device_interface_config_t& dev_config)
    {
        ESP_ERROR_CHECK(add(host_id, dev_config));
    }

    ~managed_device()
    {
        if(handle_ != nullptr)
            ESP_ERROR_CHECK(remove());
    }

    operator device() const { return device(handle_); }
};

/*
template <bool managed>
class device;

template <>
class device<true> : public managed_device
{
public:
};

template <>
class device<false> : public device_base
{
public:
    device(const device& copy_from) = default;
}; */

}}}
