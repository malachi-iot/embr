#pragma once

#include "../../../transport-v1.h"

#include <driver/twai.h>

namespace embr { namespace experimental { inline namespace v1 {

struct TwaiTransport
{

};

template <>
struct transport_traits<TwaiTransport> : transport_traits_defaults
{
    using frame_type = twai_message_t;

    static constexpr Support frame = SUPPORT_REQUIRED;
    static constexpr Support addr_in_frame = SUPPORT_REQUIRED;
};


template <>
struct transport<TRANSPORT_CAN, TRANSPORT_TRAIT_POLLED>
{

};


}}}