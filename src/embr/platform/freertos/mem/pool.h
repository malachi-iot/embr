#pragma once

#include <estd/mutex.h>

#include "../../../mem/v1/pool.h"


namespace embr { namespace mem { namespace freertos {

namespace layer1 {

template <std::size_t N, std::size_t H>
class pool : public embr::mem::v1::layer1::pool<N, H, estd::freertos::mutex<true>>
{
    using mutex_type = estd::freertos::mutex<true>;
    using base_type = embr::mem::v1::layer1::pool<N, H, mutex_type>;

public:
    // Not available until estd v0.8.11 beta2
    //pool() : base_type(estd::in_place_type_t<mutex_type>{}, estd::defer_init_t{}) {}

    estd::errc init() { return estd::errc::not_supported; }
};

}

}}}
