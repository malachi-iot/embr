#pragma once

#include <driver/i2c.h>

// For the time being we only focus on 'master' mode
// 'slave' mode I am thinking will be a 2nd transport

namespace embr { namespace experimental {
inline namespace v1 { inline namespace esp_idf { 

struct I2CTransport
{
    i2c_port_t port_;

    // NOTE: Specifically transacted
    struct mode_base
    {
        i2c_cmd_handle_t cmd;

        // DEBT: Need to sequester this specificity elsewhere
        bool ack_en = false;
        i2c_ack_type_t ack_mode = I2C_MASTER_ACK;

        // DEBT: Consider strongly to use estd::expected here instead
        esp_err_t read(uint8_t* data)
        {
            return i2c_master_read_byte(cmd, data, ack_mode);
        }

        esp_err_t read(uint8_t* data, std::size_t len)
        {
            return i2c_master_read(cmd, data, len, ack_mode);
        }

        esp_err_t write(uint8_t data)
        {
            return i2c_master_write_byte(cmd, data, ack_en);
        }

        esp_err_t write(const void* data, std::size_t len)
        {
            return i2c_master_write(cmd, (const uint8_t*) data, len, ack_en);
        }
    };
};

}

template <>
struct transport_traits<esp_idf::I2CTransport> : transport_traits_defaults
{
    static constexpr Support callback_status = SUPPORT_MAY;
    static constexpr Support per_byte = SUPPORT_OPTIONAL;

    // Undecided
    static constexpr Support static_transaction = SUPPORT_OPTIONAL;

    using transaction_type = i2c_cmd_handle_t;

    struct transaction
    {
        static i2c_cmd_handle_t begin()
        {
            return i2c_cmd_link_create();
        }

        static void end(i2c_cmd_handle_t h)
        {
            return i2c_cmd_link_delete(h);
        }
    };


    struct bus
    {
        static esp_err_t reserve(i2c_cmd_handle_t h)
        {
            return i2c_master_start(h);
        }

        static esp_err_t release(i2c_cmd_handle_t h)
        {
            return i2c_master_stop(h);
        }
    };

    struct i2c
    {

    };
};


template <>
struct mode<esp_idf::I2CTransport,
    (TransportTraits)(TRANSPORT_TRAIT_TIMEOUT |
    TRANSPORT_TRAIT_TRANSACTED)> :
    esp_idf::I2CTransport::mode_base
{
    esp_err_t commit(i2c_port_t i2c_num, estd::chrono::freertos_clock::duration timeout)
    {
        return i2c_master_cmd_begin(i2c_num, cmd, timeout.count());
    }
};



}
}}