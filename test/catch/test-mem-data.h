#pragma once

#include "embr/mem/v1/block.h"

namespace test { inline namespace mem {

using namespace embr::mem::detail::v1;

struct page
{
    unsigned sz;
    block blk;
};

// DEBT: Can't do constexpr because block flexible data member says no
// Be careful to stay on alias boundaries.
static const page pool1[]
{
    { 32, block{ block::Trivial, true } },
    { 0, block{ block::Trivial, false } }
};

}}
