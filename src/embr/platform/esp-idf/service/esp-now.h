#pragma once

#include <estd/algorithm.h>
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

    // layer1 mac
    // DEBT: Put this out into some kind of embr::layer1 namespace
    struct mac
    {
        uint8_t address[6];

        // DEBT: Finagle ourselves a constexpr here somehow
        mac(const uint8_t* copy_from)
        {
            estd::copy_n(copy_from, 6, address);
        }

        mac(const mac&) = default;

        operator const uint8_t*() const { return address; }
    };

    // These events are not ISR-bound, but they do happen within the WiFi
    // stack and need to be light on their feet like an ISR
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

    struct recv_info
    {
        // It seems the source and dest are temporary/local variables too
        //const uint8_t* source;
        //const uint8_t* dest;

        mac source, dest;

        wifi_pkt_rx_ctrl_t rx_ctrl;

        recv_info(
            const mac& source, const mac& dest,
            const wifi_pkt_rx_ctrl_t& rx_ctrl) :
            source{source},
            dest{dest},
            rx_ctrl{rx_ctrl}
        {}

        recv_info(
            const mac& source, const mac& dest,
            const wifi_pkt_rx_ctrl_t& rx_ctrl,
            const uint8_t* copy_from_data, size_t sz) :
            source{source},
            dest{dest},
            rx_ctrl{rx_ctrl}
        {
            estd::copy_n(copy_from_data, sz, data);
        }

        recv_info(const event::receive& copy_from) :
            source(copy_from.info.src_addr),
            dest(copy_from.info.des_addr),
            rx_ctrl(*copy_from.info.rx_ctrl)
        {
            estd::copy_n(copy_from.data.data(), copy_from.data.size(), data);
        }

        uint8_t data[];
    };


    struct send_info
    {
        mac dest;
        const esp_now_send_status_t status;
    };

    EMBR_SERVICE_RUNTIME_BEGIN(SparseService)

    state_result on_start();

    static void callback(const esp_now_recv_info_t*, const uint8_t*, int);
    static void callback(const uint8_t* mac_addr, esp_now_send_status_t);

    EMBR_SERVICE_RUNTIME_END
};

}}

}