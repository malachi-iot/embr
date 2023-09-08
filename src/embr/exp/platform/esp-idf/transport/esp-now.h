#pragma once

#include "../../../transport-v1.h"

#include <esp_now.h>

// For the time being we only focus on 'master' mode
// 'slave' mode I am thinking will be a 2nd transport

namespace embr { namespace experimental {
inline namespace v1 { inline namespace esp_idf {

struct EspNowTransport
{
    using endpoint = const uint8_t*;

    struct mode_base
    {
        esp_err_t read(esp_now_recv_cb_t cb)
        {
            return esp_now_register_recv_cb(cb);
        }

        esp_err_t write(const void* data, std::size_t len, endpoint peer_addr)
        {
            return esp_now_send(peer_addr, (const uint8_t*)data, len);
        }
    };
};

}

template<>
struct transport_traits<esp_idf::EspNowTransport> : transport_traits_defaults
{
    static constexpr Support callback_rx = SUPPORT_REQUIRED;
    static constexpr Support callback_tx = SUPPORT_OPTIONAL;

    // Undecided
    static constexpr Support callback_global = SUPPORT_REQUIRED;
    static constexpr Support callback_instance = SUPPORT_MUST_NOT;
};

}
}}