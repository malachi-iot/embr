#pragma once

#include "embr/mem/v1/block.h"

// DEBT: See if we can scoop this from estd
struct SideEffector
{
    int* counter_{};

    SideEffector(SideEffector&& move_from) :
        counter_{move_from.counter_}
    {
        move_from.counter_ = nullptr;
    }

    explicit SideEffector(int* counter) : counter_{counter}
    {
        ++*counter_;
    }

    ~SideEffector()
    {
        if(counter_)   --*counter_;
    }
};


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

static const page pool2[]
{
    { 24, block{ block::Trivial, true } },
    { 72, block{ block::Trivial, false} },
    { 64, block{ block::Trivial, true } },
    { 8 , block{ block::Trivial, false} },
    { 48, block{ block::Trivial, true } },
    { 40, block{ block::Trivial, true } },
    { 0 , block{ block::Trivial, false} }
};

}}
