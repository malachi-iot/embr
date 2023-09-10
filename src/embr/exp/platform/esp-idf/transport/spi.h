#pragma once

#include <driver/spi_master.h>

// For the time being we only focus on 'master' mode
// 'slave' mode I am thinking will be a 2nd transport

namespace embr { namespace experimental {
inline namespace v1 { inline namespace esp_idf {

struct SpiMasterTransport
{
    spi_device_handle_t handle;

    struct Command
    {
        uint16_t cmd;
        uint32_t addr;
    };
};

}

template <>
struct transport_traits<esp_idf::SpiMasterTransport> : transport_traits_defaults
{
    using transport_type = esp_idf::SpiMasterTransport;
    using transaction_type = spi_transaction_t;

    static constexpr Support transport_command = SUPPORT_MAY;
    static constexpr Support transport_subaddr = SUPPORT_MAY;
    static constexpr Support instance_transact_callback = SUPPORT_MAY;

    struct transaction
    {
        static transaction_type begin()
        {
            return {};
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

    esp_err_t read(spi_transaction_t* trans_desc, void* data, std::size_t len, void* instance = nullptr)
    {
        trans_desc->rxlength = len;
        trans_desc->rx_buffer = data;
        trans_desc->user = instance;

        return spi_device_queue_trans(handle, trans_desc, portMAX_DELAY);
    }

    esp_err_t write(spi_transaction_t* trans_desc, void* data, std::size_t len, void* instance = nullptr)
    {
        trans_desc->length = len;
        trans_desc->tx_buffer = data;
        trans_desc->user = instance;

        return spi_device_queue_trans(handle, trans_desc, portMAX_DELAY);
    }


    esp_err_t read(spi_transaction_t* trans_desc, Command command, void* data, std::size_t len, void* instance = nullptr)
    {
        trans_desc->cmd = command.cmd;
        return read(trans_desc, data, len, instance);
    }
};

}
}}