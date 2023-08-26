#pragma once

#include <estd/span.h>

#include <esp_now.h>

#include "core.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

struct EspNow : embr::SparseService
{
    constexpr static const char* TAG = "ESP-NOW";
    constexpr static const char* name() { return TAG; }

    using this_type = EspNow;

    // DEBT: Really need some kind of singleton enforcer
    template <class TSubject>
    using static_type = static_factory<TSubject, this_type>::static_type;

    struct event
    {
        struct receive
        {
            const esp_now_recv_info_t& info;
            const estd::span<const uint8_t> data;
        };

        struct send
        {
            const estd::span<const uint8_t, 6> mac;
            const esp_now_send_status_t status;
        };
    };

    EMBR_SERVICE_RUNTIME_BEGIN(SparseService)

    state_result on_start();

    static void callback(const esp_now_recv_info_t*, const uint8_t*, int);
    static void callback(const uint8_t* mac_addr, esp_now_send_status_t);

    EMBR_SERVICE_RUNTIME_END
};

}}

}