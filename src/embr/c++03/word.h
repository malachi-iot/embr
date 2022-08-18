#pragma once

#include "../internal/type_from_bits.h"

namespace embr { namespace internal {

template <size_t bits, bool is_signed, word_strictness strict>
class word_base : public type_from_bits<bits, is_signed>
{
    typedef type_from_bits<bits, is_signed> base_type;
};

}}