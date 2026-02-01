#pragma once

#include <estd/mutex.h>

#include "../../../mem/v1/pool.h"


namespace embr { namespace mem { namespace freertos {

namespace layer1 {

template <std::size_t N, std::size_t H>
using pool = embr::mem::v1::layer1::pool<N, H, estd::freertos::mutex<true>>;

}

}}}
