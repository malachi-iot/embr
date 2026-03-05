#pragma once

#include "embr/mem/v1/block.h"
#include "embr/mem/v1/unit.h"

// DEBT: See if we can scoop this from estd
struct SideEffector
{
    int* counter_{};
    int copied_to_counter{};
    mutable int copied_from_counter : 4;
    int moved_from_counter : 4;
    int moved_to_counter : 4;
    bool constructed_ : 1;

    // TODO: Add a 'moved_to' pointer, useful for debugging.  Not doing so yet
    // because I think some of our tests have calcified into specific size of SideEffector

    constexpr SideEffector() :
        copied_from_counter{},
        moved_from_counter{},
        moved_to_counter{},
        constructed_{true}
    {}

    constexpr SideEffector(const SideEffector& copy_from) :
        counter_{copy_from.counter_},
        copied_from_counter{},
        moved_from_counter{},
        moved_to_counter{},
        constructed_{true}
    {
        ++copy_from.copied_from_counter;
        ++copied_to_counter;
    }

    constexpr SideEffector(SideEffector&& move_from) noexcept :
        counter_{move_from.counter_},
        copied_from_counter{},
        moved_from_counter{},
        moved_to_counter{},
        constructed_{true}
    {
        ++move_from.moved_from_counter;
        move_from.counter_ = nullptr;
        ++moved_to_counter;
    }

    explicit SideEffector(int* counter) :
        counter_{counter},
        copied_from_counter{},
        moved_from_counter{},
        moved_to_counter{},
        constructed_{true}
    {
        ++*counter_;
    }

    ~SideEffector()
    {
        if(counter_)   --*counter_;
    }

    constexpr unsigned counter() const { return *counter_; }
};


static_assert(std::is_trivially_constructible<SideEffector>::value == false);

// Semi-CRTP style
template <class Self>
class RttoBase : public estd::internal::rtto_base::base
{
    using base_type = estd::internal::rtto_base::base;

public:
    RttoBase() :
        base_type(estd::internal::rtto<Self>::utility)
    {}
};

// DEBT: This helper seems useful enough to put into estd proper
template <class Self>
class RttoVirtualWrap : public estd::internal::rtto_base::virtual_base, public Self
{
    using rtto = estd::internal::rtto<RttoVirtualWrap>;

public:
    template <class ...Args>
    constexpr explicit RttoVirtualWrap(Args&&...args) : Self(std::forward<Args>(args)...)    {}

    int move_to(void* dest, int sz) override
    {
        return rtto::move(this, dest, sz);
    }

    int copy_to(void* dest, int sz) const override
    {
        return rtto::copy(this, dest, sz);
    }

#if FEATURE_ESTD_RTTO_GET_METADATA
    // DEBT: Change estd signature to return const metadata*
    int get_metadata(const metadata** out) const override
    {
        *out = rtto::get_metadata();
        return 0;
    }
#endif

    ~RttoVirtualWrap() override = default;
};


class RttoSideEffector :
    public RttoBase<RttoSideEffector>,  // This guy must come first
    public SideEffector
{
    using base_type = SideEffector;

public:
    template <class ...Args>
    explicit RttoSideEffector(Args&&...args) : base_type(std::forward<Args>(args)...)    {}
};

using RttoVirtSideEffector = RttoVirtualWrap<SideEffector>;

// NOTE: Considered making an operators.h for this guy.  Somehow putting him at global scope still
// doesn't feel right - in part because this hangs off standard bytes tag and I don't want to presume
// this is how everyone wants to render 'bytes'
namespace estd { namespace units { inline namespace v1 { namespace detail {

// ADL you are a demanding one.  OK, here you go
//template <class Traits>
//std::ostream& operator<<(std::ostream& out, const unit<Traits>& v)
template <class Rep, class Period>
std::ostream& operator<<(std::ostream& out, embr::mem::bytes_unit<Rep, Period> v)
{
    // TODO: Make a helper for this in estd, perhaps another flag for put_unit
    if(Period::num == Period::den || v.count() == 0)
        out << put_unit(bytes<int>(v));
    else
        out << v.count() << " (" << put_unit(bytes<int>(v)) << ')';

    return out;
}

}}}}




namespace test { inline namespace mem {

using namespace embr::mem::detail::v1;

using block = block_8;

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

static const page pool11[]
{
    { /* 0 */ 48, block{ block::Trivial, A } },
    { /* 1 */ 80, block{ block::Trivial, A } },
    { /* 2 */ 16, block{ block::Trivial, A } },
    { /* 3 */ 24, block{ block::Trivial, F } },
    { /* 4 */ 32, block{ block::RttoProxy, A } },
    { /* 5 */ 0 , block{ block::Trivial, F } },
};

static const page pool12[]
{
    { /* 0 */ 72, block{ block::Trivial, A } },
    { /* 1 */ 16, block{ block::Trivial, A } },
    { /* 2 */ 32, block{ block::RttoProxy, A } },
    { /* 3 */ 32, block{ block::RttoBase, A } },
    { /* 4 */ 32, block{ block::Trivial, F } },
    { /* 5 */ 32, block{ block::RttoBase, A } },
    { /* 6 */ 0 , block{ block::Trivial, F } },
};

static const page pool13[]
{
    { /* 0 */ 24, block{ block::Trivial, F } },
    { /* 1 */ 32, block{ block::RttoBase, A } },
    { /* 2 */ 16, block{ block::Trivial, F } },
    { /* 3 */ 24, block{ block::Trivial, A } },
    { /* 4 */ 56, block{ block::Trivial, A } },
    { /* 5 */ 32, block{ block::RttoBase, A } },
    { /* 6 */ 32, block{ block::RttoProxy, A } },
    { /* 7 */ 168, block{ block::Trivial, F } },
    { /* 8 */ 0 , block{ block::Trivial, F } },
};

}}
