#pragma once

#include "spi/streambuf.h"

namespace embr { namespace esp_idf {

namespace spi {


struct bus
{
    const spi_host_device_t host_id;

    constexpr bus(spi_host_device_t host_id) : host_id(host_id) {}

    inline esp_err_t initialize(const spi_bus_config_t& bus_config, spi_common_dma_t dma_chan) const
    {
        return spi_bus_initialize(host_id, &bus_config, dma_chan);
    }

    inline esp_err_t free() const
    {
        return spi_bus_free(host_id);
    }

    inline device add(const spi_device_interface_config_t& dev_config)
    {
        return device(host_id, dev_config);
    }

    inline void remove(const device& d)
    {
        spi_bus_remove_device(d);
    }

    constexpr operator spi_host_device_t() const { return host_id; }
};

}

}}