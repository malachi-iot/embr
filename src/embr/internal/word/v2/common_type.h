#pragma once

#include "fwd.h"
#include <estd/internal/fwd/common_type.h>

namespace estd {

template <size_t bits, embr::v2::word_options o, class T>
struct common_type<embr::v2::word<bits, o>, T> :
    estd::common_type<typename embr::v2::word<bits, o>::type, T>
{};

template <size_t bits, embr::v2::word_options o, class T>
struct common_type<T, embr::v2::word<bits, o>> : common_type<embr::v2::word<bits, o>, T> {};


}
