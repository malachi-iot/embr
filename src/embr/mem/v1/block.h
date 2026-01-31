#pragma once

#include <estd/internal/rtto.h>
#include <estd/units.h>

#include "enum.h"
#include "error.h"
#include "fwd.h"
#include "traits.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

struct block_base_uint8 : block_mode_enum, handles_traits_uint8 {};

class block_diagnostic;

class alignas(void*) block_header_8 : public block_base_uint8
{
    using this_type = block_header_8;

protected:
    template <class T>
    using rtto = estd::internal::rtto<T>;

    template <class HandlesTraits, class Block>
    friend class bundle_base;

    // DEBT: rtto base is WAY overloaded.  Needs attention
    using rtto_base_type = estd::internal::rtto_base::base;
    using rtto_proxy = estd::internal::rtto_base::rtto_base::proxy<>;

    struct alignas(void*)
    {
        unsigned prev_ : 8;
        unsigned next_ : 8;
        modes mode_ : 2;
        bool allocated_ : 1;
        unsigned lock_count_ : 4;
        unsigned ref_count_ : 4;
    };

    // DEBT: Apparently this and flexible array is a GCC extension.
    char data_[0];

public:
    block_header_8() = default;
    explicit block_header_8(modes mode, bool allocated,
        unsigned prev = null, unsigned next = null) :
        prev_{prev},
        next_{next},
        mode_{mode},
        allocated_{allocated},
        lock_count_{0},
        ref_count_{0}
    {}

    block_header_8(const this_type&) = default;

    this_type& operator=(const this_type&) = default;
    this_type& operator=(this_type&&) = default;

    constexpr modes mode() const { return mode_; }
    constexpr handle_type prev() const { return prev_; }
    constexpr handle_type next() const { return next_; }
    constexpr bool allocated() const { return allocated_; }
    constexpr unsigned lock_count() const { return lock_count_; }
    constexpr bool reserved() const { return lock_count_ == 0xF; }

    invariant_result invariant() const
    {
        EMBR_MEM_INVARIANT_ASSERT((prev_ == null && next_ == null) || prev_ != next_,
            "Circular linked list not allowed", "");

        return {};
    }

    // FIX: Probably not 100% right, because RttoBase mode includes size of rtto::u_ in the object itself
    static constexpr estd::units::bytes<unsigned> header_size(modes mode)
    {
        return estd::units::bytes<unsigned>((mode == Trivial || mode == RttoBase) ?
            sizeof(this_type) :
            (sizeof(this_type) + sizeof(estd::internal::rtto_base::base)));
    }

    // DEBT: Protect this and make friend classes, or pull WriteableBlock child stunt
    void reset(modes mode, bool allocated)
    {
        mode_ = mode;
        allocated_ = allocated;
        lock_count_ = 0;
        ref_count_ = 0;
    }
};


class alignas(void*) block_8 : public block_header_8
{
    using base_type = block_header_8;
    using this_type = block_8;

    friend class block_diagnostic;

    // DEBT: data[0] is a GCC extension, according to AI

public:
    block_8() = default;

    explicit block_8(modes mode, bool allocated,
        unsigned prev = null, unsigned next = null) :
        base_type(mode, allocated, prev, next)
    {}

    ESTD_CPP_DEFAULT_RULE_OF_5(block_8)

    void* data() { return data_; }
    constexpr const void* data() const { return data_; }
    rtto_proxy* proxy() { return (rtto_proxy*) data_; }
    rtto_base_type* rtto_base() { return (rtto_base_type*) data_; }

    template <class T, class ...Args>
    void emplace_rtto_proxied(Args&&...args);

    template <class T, class ...Args>
    void emplace(Args&&...args);

    // Destroy tracked object, not necessarily block itself
    void destroy();

    void move_from(this_type* from, unsigned sz);
};

// Block is NOT packed since following data wants to sit comfortably on aligned pointer boundary
class block_diagnostic
{
    block_8 b{};

    static_assert(offsetof(block_8, data_) == sizeof(void*));
    static_assert(sizeof(block_8) == sizeof(void*));
};


// EXPERIMENTAL
class alignas(void*) block_6 : block_base_uint8
{
    struct alignas(void*)
    {
        unsigned prev_ : 6;
        modes mode_ : 2;
        unsigned next_ : 6;
        bool allocated_: 1;
        bool locked_ : 1;
    };
public:
};

}}

}}
