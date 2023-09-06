#pragma once

#include <estd/chrono.h>

#include "../../../transport-v1.h"

#include <driver/twai.h>

namespace embr { namespace experimental { inline namespace v1 {

struct TwaiTransport
{

};

template <>
struct error<TwaiTransport, TRANSPORT_RET_OK> :
    estd::integral_constant<esp_err_t, ESP_OK> {};

template <>
struct transport_traits<TwaiTransport> : transport_traits_defaults
{
    using frame_type = twai_message_t;
    using transport_type = TwaiTransport;

    template <ReturnStatus rs>
    using error_type = error<transport_type, rs>;

    template <TransportTraits tt>
    using mode = v1::mode<transport_type, tt>;

    static constexpr Support frame = SUPPORT_REQUIRED;
    static constexpr Support addr_in_frame = SUPPORT_REQUIRED;

    struct can
    {
        static constexpr Support eid_in_frame = SUPPORT_REQUIRED;   // Flag to indicate extended 29 bit ID
        static constexpr Support rtr_in_frame = SUPPORT_REQUIRED;
        static constexpr Support ss_in_frame = SUPPORT_REQUIRED;

        static void rtr(frame_type& f, bool on = true)
        {
            f.rtr = on;
        }

        static void single_shot(frame_type& f, bool on = true)
        {
            f.ss = on;
        }

        static void extended_id(frame_type& f, bool on = true)
        {
            f.extd = on;
        }
    };
};


template <>
struct mode<TwaiTransport, TRANSPORT_TRAIT_BLOCKING>
{
    
};

template <>
struct mode<TwaiTransport, TRANSPORT_TRAIT_TIMEOUT>
{
    esp_err_t send(const twai_message_t& message, estd::chrono::freertos_clock::duration d)
    {
        return ESP_OK;
    }
};


template <>
struct mode<TwaiTransport, TRANSPORT_TRAIT_POLLED>
{
    
};

template <>
struct transport<TRANSPORT_CAN, TRANSPORT_TRAIT_POLLED>
{

};


}}}