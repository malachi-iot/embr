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
    unsigned phys_sz;
    block blk;
};

// DEBT: Can't do constexpr because block flexible data member says no
// Be careful to stay on alias boundaries.
static const page pool1[]
{
    { 32, block{ block::Trivial, true } },
    { 0, block{ block::Trivial, false } }
};

static constexpr bool A = true;
static constexpr bool F = false;

static const page pool2[]
{
    { /* 0 */ 24, block{ block::Trivial, A } },
    { /* 1 */ 72, block{ block::Trivial, F } },
    { /* 2 */ 64, block{ block::Trivial, A } },
    { /* 3 */ 8,  block{ block::Trivial, F } },     // NOTE: This is an invalid block size
    { /* 4 */ 48, block{ block::Trivial, A } },
    { /* 5 */ 40, block{ block::Trivial, A } },
    { /* 6 */ 0,  block{ block::Trivial, F } }
};

static const page pool3[]
{
    { /* 0 */ 48, block{ block::Trivial, A } },
    { /* 1 */ 16, block{ block::Trivial, F } },
    { /* 2 */ 40, block{ block::Trivial, A } },
    { /* 3 */ 48, block{ block::Trivial, F } },
    { /* 4 */ 88, block{ block::Trivial, A } },
    { /* 5 */ 0,  block{ block::Trivial, F } },
};

// tested: reverse non-overlapping trivial
static const page pool4[]
{
    { /* 0 */ 24,  block{ block::Trivial, F } },
    { /* 1 */ 80,  block{ block::Trivial, A } },
    { /* 2 */ 160, block{ block::Trivial, F, {}, 6 } },
    { /* 6 */ 88,  block{ block::Trivial, A } },
    { /* 7 */ 0 ,  block{ block::Trivial, F } }
};

static const page pool5[]
{
    { /* 0 */ 72,  block{ block::Trivial, A } },
    { /* 1 */ 48,  block{ block::Trivial, A } },
    { /* 2 */ 56,  block{ block::Trivial, A } },
    { /* 3 */ 144, block{ block::Trivial, F, {}, 6 } },
    { /* 6 */ 88,  block{ block::Trivial, A } },
    { /* 7 */ 0 ,  block{ block::Trivial, F } }
};

static const page pool6[]
{
    { /* 0 */ 88, block{ block::Trivial, F } },
    { /* 1 */ 24, block{ block::Trivial, A } },
    { /* 2 */ 40, block{ block::Trivial, F } },
    { /* 3 */ 72, block{ block::Trivial, A } },
    { /* 4 */ 24, block{ block::Trivial, F } },
    { /* 5 */ 56, block{ block::Trivial, A } },
    { /* 6 */ 88, block{ block::Trivial, A } },
    { /* 7 */ 0 , block{ block::Trivial, F } },
};

static const page pool7[]
{
    { /* 0 */ 64, block{ block::Trivial, F } },
    { /* 1 */ 16, block{ block::Trivial, A } },
    { /* 2 */ 40, block{ block::Trivial, F } },
    { /* 3 */ 48, block{ block::Trivial, A } },
    { /* 4 */ 48, block{ block::Trivial, A } },
    { /* 6 */ 0 , block{ block::Trivial, F } }
};

static const page pool8[]
{
    { /* 0 */ 96, block{ block::Trivial, F, {}, 2 } },
    { /* 2 */ 48, block{ block::Trivial, A } },
    { /* 3 */ 16, block{ block::Trivial, F } },
    { /* 4 */ 16, block{ block::Trivial, A } },
    { /* 5 */ 40, block{ block::Trivial, F } },
    { /* 6 */ 32, block{ block::Trivial, A } },
    { /* 7 */ 0 , block{ block::Trivial, F } }
};

static const page pool9[]
{
    { /* 0 */ 56, block{ block::Trivial, A } },
    { /* 1 */ 32, block{ block::Trivial, A } },
    { /* 2 */ 80, block{ block::Trivial, A } },
    { /* 3 */ 16, block{ block::Trivial, F } },
    { /* 4 */ 32, block{ block::Trivial, A } },
    { /* 5 */ 16, block{ block::Trivial, F } },
    { /* 6 */ 40, block{ block::Trivial, A } },
    { /* 7 */ 0 , block{ block::Trivial, F } }
};

static const page pool10[]
{
    { /* 0 */ 88, block{ block::Trivial, F } },
    { /* 1 */ 24, block{ block::Trivial, A } },
    { /* 2 */ 40, block{ block::Trivial, F } },
    { /* 3 */ 72, block{ block::Trivial, A } },
    { /* 4 */ 0 , block{ block::Trivial, F } }
};

}}
