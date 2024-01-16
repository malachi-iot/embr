#pragma once

#include <embr/platform/esp-idf/traits.h>
#include "../peripherals.h"

namespace embr { namespace esp_idf { namespace traits { inline namespace v1 {

namespace internal {

struct board
{
    static constexpr const char* vendor = "Unspecified";
    static constexpr const char* name = "Generic";
    static constexpr const char* chip = chip_traits<>::name();

protected:
    template <class ...Mux>
    using mux = estd::variadic::types<Mux...>;

public:
    using io = mux<>;
};

}

}}}}