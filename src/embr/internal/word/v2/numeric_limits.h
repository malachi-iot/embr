#pragma once

#include "fwd.h"

#include <estd/limits.h>
#include <estd/internal/macro/push.h>

namespace estd {

template <size_t bits, embr::v2::word_options o>
struct numeric_limits<embr::v2::word<bits, o> > :
    numeric_limits<typename embr::v2::word<bits, o>::type>
{
    using word_type = embr::v2::word<bits, o>;
    using type = typename word_type::type;

    static constexpr bool is_signed = o & embr::v2::word_options::is_signed;
    static constexpr int digits = bits;

    static constexpr unsigned long long range = 1ULL << bits;

    // 2's complement

    static constexpr type min()
    {
        return static_cast<type>(is_signed ? -(range / 2) : 0);
    }

    static constexpr type max()
    {
        return static_cast<type>(is_signed ? ((range - 1) / 2) : (range - 1));
    }
};


}

#include <estd/internal/macro/pop.h>
