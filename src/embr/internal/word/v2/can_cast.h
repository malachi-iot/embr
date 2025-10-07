#pragma once

#include "../../can_cast.h"

#include "endian.h"
#include "fwd.h"

namespace embr { namespace internal {

// NOTE: For the time being, we can expect all 'word' cast attempts to place 'word' on the left

// DEBT: Name mild collision with can_cast
template <v2::word_options o, v2::word_options o2, class Enabled = void>
struct is_castable : estd::bool_constant<false> {};

// Presumed that bits match already
// Does NOT pay attention to sign, you are on your own there
template <v2::word_options o, v2::word_options o2>
struct is_castable<
    o, o2,
    estd::enable_if_t<
        is_matching_endian<o, o2>::value &&
        // DEBT: With matching endianness and resolved type, a packed and non packed could in fact be castable
        !is_set((o ^ o2) & v2::word_options::packed)>> :
    estd::bool_constant<true>
{};

template <size_t bits, v2::word_options o, v2::word_options o2, class Period>
struct can_cast<
    v2::word<bits, o>,
    estd::chrono::duration<v2::word<bits, o2>, Period>,
    estd::enable_if_t<is_castable<o, o2>::value>> :
    estd::bool_constant<true>
{
};


template <size_t bits, v2::word_options o, v2::word_options o2>
struct can_cast<
    v2::word<bits, o>,
    v2::word<bits, o2>,
    estd::enable_if_t<is_castable<o, o2>::value>> :
    estd::bool_constant<true>
{
};

}}
