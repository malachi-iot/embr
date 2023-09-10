#pragma once

#include <driver/spi_master.h>

// For the time being we only focus on 'master' mode
// 'slave' mode I am thinking will be a 2nd transport

namespace embr { namespace experimental {
inline namespace v1 { inline namespace esp_idf {

struct SpiMasterTransport
{
    spi_device_handle_t handle;
};

}

template <>
struct transport_traits<esp_idf::SpiMasterTransport> : transport_traits_defaults
{
    using transport_type = esp_idf::SpiMasterTransport;
    using transaction_type = spi_transaction_t;

    static constexpr Support transport_command = SUPPORT_MAY;
    static constexpr Support transport_subaddr = SUPPORT_MAY;

    struct transaction
    {
        static void begin(transaction_type)
        {

        }

        static void end(transaction_type)
        {

        }


        static esp_err_t commit(const transport_type& tp, transaction_type* t)
        {
            return spi_device_queue_trans(tp.handle, t, portMAX_DELAY);
        }
    };

    struct bus
    {
        static esp_err_t reserve(const transport_type& t)
        {
            return spi_device_acquire_bus(t.handle, portMAX_DELAY);
        }

        static void release(const transport_type& t)
        {
            spi_device_release_bus(t.handle);
        }
    };
};


template <>
struct mode<esp_idf::SpiMasterTransport, TRANSPORT_TRAIT_TRANSACTED> :
    esp_idf::SpiMasterTransport
{
    using traits = transport_traits<esp_idf::SpiMasterTransport>;

    esp_err_t read(spi_transaction_t* trans_desc)
    {
        return spi_device_queue_trans(handle, trans_desc, portMAX_DELAY);
    }
};

}
}}