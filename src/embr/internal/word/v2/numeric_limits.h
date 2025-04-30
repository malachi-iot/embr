#pragma once

#include "fwd.h"

#include <estd/limits.h>

namespace estd {

template <size_t bits, embr::v2::word_options o>
struct numeric_limits<embr::v2::word<bits, o> > :
    numeric_limits<typename embr::v2::word<bits, o>::type>
{
    using word_type = embr::v2::word<bits, o>;
    using type = typename word_type::type;

    static constexpr type min() { return 0; }
    static constexpr type max() { return word_type::mask(); }

    static constexpr bool is_signed = o & embr::v2::word_options::is_signed;
    static constexpr int digits = bits;
};


}
