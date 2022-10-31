#include "../../obsolete/dataport.h"
#include "transport.hpp"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#endif

#pragma once

namespace embr { namespace lwip {

/// Makes an lwip-specific UDP dataport
template <class TSubject>
auto make_udp_dataport(TSubject& s, uint16_t listen_port, uint16_t source_port = 0) -> 
    decltype(embr::make_dataport<experimental::UdpDataportTransport>(s, listen_port, source_port))
{
    return embr::make_dataport<experimental::UdpDataportTransport>(s, listen_port, source_port);
}


// More or less an Lwip dataport observer, customized for ESP logging
template <class TPolicy>
struct DiagnosticDataportObserver
{
    static constexpr const char* TAG = "DiagnosticDataportObserver";

    typedef embr::DataPortEvents<TPolicy> event;

    template <bool use_addr_ptr>
    static const char* get_addr_str(const embr::lwip::internal::Endpoint<use_addr_ptr>& endpoint)
    {
        // https://www.nongnu.org/lwip/2_1_x/group__ipaddr.html
        // careful, not reentrant--
        const ip_addr_t* addr = endpoint.address();
        const char* addr_str = ipaddr_ntoa(addr);

        return addr_str;
    }

    static void on_notify(const typename event::receive_queued& e)
    {
        const char* addr_str = get_addr_str(e.item.addr());
        uint16_t port = e.item.addr().port();
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "on_notify(receive_queued): from=%s:%u", addr_str, port);
#endif
    }

    static void on_notify(const typename event::receive_dequeuing& e)
    {
        const char* addr_str = get_addr_str(e.item.addr());
        uint16_t port = e.item.addr().port();
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "on_notify(receive_dequeuing): from=%s:%u", addr_str, port);
#endif
    }

    static void on_notify(const typename event::send_queued& e)
    {
        const char* addr_str = get_addr_str(e.item.addr());
        uint16_t port = e.item.addr().port();
#ifdef ESP_PLATFORM
        ESP_LOGI(TAG, "on_notify(send_queued): to=%s:%u", addr_str, port);
#endif
    }
};



}}