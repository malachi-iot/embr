#pragma once

#include <estd/internal/rtto.h>
#include <estd/units.h>

#if FEATURE_STD_OSTREAM
#include <ostream>
#endif

#include "enum.h"
#include "error.h"
#include "feature.h"
#include "fwd.h"
#include "traits.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

namespace mixin {

template <class Derived>
class block_accessors
    // Enabling this brings about warnings for block_diagnostic offsetof
    //: public block_mode_enum
{
    //using modes = block_mode_enum::modes;
};

template <class Derived>
class block_invariant
// Enabling this brings about warnings for block_diagnostic offsetof
//: public block_mode_enum
{
    //using modes = block_mode_enum::modes;
};

}

struct block_base_uint : block_mode_enum
{
    using bytes = estd::units::bytes<unsigned>;

    // How much extra allocation is needed for this block to accomodate rtto.  Note
    // that RttoBase and RttoVirtual being an "is a" have already allocated that space,
    // so size is 0.
    static constexpr bytes rtto_overhead(modes mode)
    {
        return bytes(mode == RttoProxy ?
            // DEBT: This is too "just gotta know" - make something like
            // rtto_base::proxy_size
            sizeof(estd::internal::rtto_base::base) : 0);
    }
};

class block_diagnostic;

template <ESTD_CPP_CONCEPT(HandlesTraits) HandlesTraits>
class alignas(void*) block_header_base :
    public HandlesTraits,
    public block_base_uint,
    public mixin::block_accessors<block_header_base<HandlesTraits>>
{
    using this_type = block_header_base;

public:
    using traits_type = HandlesTraits;

    using typename traits_type::handle_type;
    using traits_type::null;

protected:
    template <class T>
    using rtto = estd::internal::rtto<T>;

    template <class HandlesTraits2, class Block>
    friend struct bundle_base;

    // DEBT: rtto base is WAY overloaded.  Needs attention
    using rtto_base_type = estd::internal::rtto_base::base;
    using rtto_proxy = estd::internal::rtto_base::rtto_base::proxy<>;
    using rtto_virt = estd::internal::rtto_base::virtual_base;

    // To control bit placement, we need to manually manage a few flags
    // 04OCT26 DEBT: Probably don't need to do this actually, since bit field is
    // stable *enough*
    struct flags
    {
        uint8_t flags_;
        unsigned lock_count_ : 4;
        unsigned ref_count_ : 4;

        static constexpr unsigned modes_mask = 0x03;
        static constexpr unsigned allocated_pos = 3;
        static constexpr unsigned ext_pos = 4;

        constexpr modes mode() const { return static_cast<modes>(flags_ & modes_mask); }
        void mode(modes m)
        {
            flags_ = (flags_ & ~modes_mask) | static_cast<unsigned>(m);
        }
        constexpr bool allocated() const
        {
            return static_cast<modes>(flags_ >> allocated_pos);
        }

    }   __attribute__((packed));

    struct alignas(void*)
    {
        modes mode_ : 2;
        bool allocated_ : 1;
        bool ext_ : 1;              // Flag to indicate 8 or 16 bit mode (EXPERIMENTAL, INACTIVE)
        unsigned lock_count_ : 4;
        unsigned ref_count_ : 4;
        handle_type prev_, next_;

    }   __attribute__((packed));

    // DEBT: Apparently this and flexible array is a GCC extension.
    char data_[0];

public:
    block_header_base() = default;
    explicit constexpr block_header_base(modes mode, bool allocated,
        handle_type prev = null, handle_type next = null) :
        mode_{mode},
        allocated_{allocated},
        ext_{false},
        lock_count_{0},
        ref_count_{0},
        prev_{prev},
        next_{next},
        data_{}         // Just a compiler formality.  Obviously not doing anything
    {}

    block_header_base(const this_type&) = default;

    this_type& operator=(const this_type&) = default;
    this_type& operator=(this_type&&) = default;

    constexpr modes mode() const { return mode_; }
    constexpr handle_type prev() const { return prev_; }
    constexpr handle_type next() const { return next_; }
    constexpr bool allocated() const { return allocated_; }
    constexpr unsigned lock_count() const { return lock_count_; }

    // NOT USED and probably not correct, tricky to make this atomic since block itself can move around.
    // That makes it difficult since we want to use this specifically at assess/defrag which ultimately
    // may indeed move the block
    constexpr bool reserved() const { return lock_count_ == 0xF; }

    invariant_result invariant() const
    {
        EMBR_MEM_INVARIANT_ASSERT((prev_ == null && next_ == null) || prev_ != next_,
            "Circular linked list not allowed", "");

        return {};
    }

    static constexpr bytes header_size(modes mode)
    {
        // Due to https://github.com/malachi-iot/estdlib/issues/193 needing to wrap
        // sizeof(this_type)
        return rtto_overhead(mode) + bytes(sizeof(this_type));
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

using block_header_8 = block_header_base<handles_traits_uint8>;

class alignas(void*) block_8 : public block_header_8
{
    using base_type = block_header_8;
    using this_type = block_8;

    friend class block_diagnostic;

    // DEBT: data[0] is a GCC extension, according to AI

public:
    block_8() = default;

    using metadata_type = const estd::internal::rtto_base::metadata*;

    constexpr explicit block_8(modes mode, bool allocated,
        handle_type prev = null, handle_type next = null) :
        base_type(mode, allocated, prev, next)
    {}

    ESTD_CPP_DEFAULT_RULE_OF_5(block_8)

    void* data() { return data_; }
    constexpr const void* data() const { return data_; }
    rtto_proxy* proxy() { return (rtto_proxy*) data_; }
    const rtto_proxy* proxy() const { return (rtto_proxy*) data_; }
    rtto_base_type* rtto_base() { return (rtto_base_type*) data_; }
    const rtto_base_type* rtto_base() const { return (rtto_base_type*) data_; }
    rtto_virt* virt() { return (rtto_virt*) data_; }
    const rtto_virt* virt() const { return (rtto_virt*) data_; }

    template <class T, class ...Args>
    void emplace_rtto_proxied(Args&&...args);

    template <class T, class ...Args>
    void emplace(Args&&...args);

    metadata_type metadata() const;

    // Destroy tracked object, not necessarily block itself
    void destroy();

    void copy_from(this_type* from, unsigned sz);
    void move_from(this_type* from, unsigned sz);
};

// Block is NOT packed since following data wants to sit comfortably on aligned pointer boundary
class block_diagnostic
{
    [[maybe_unused]] block_8 b{};

    static_assert(sizeof(block_8::flags) == 2);
    static_assert(offsetof(block_8, data_) == sizeof(void*));
    static_assert(sizeof(block_8) == sizeof(void*));
};


// EXPERIMENTAL
class alignas(void*) block_6 : block_base_uint, handles_traits_uint_base<uint8_t>
{
    struct alignas(void*)
    {
        unsigned prev_ : 6;
        modes mode_ : 2;
        unsigned next_ : 6;
        unsigned locked_or_allocated_ : 2;
    };
public:

    // 0, 1 or 2
    constexpr unsigned lock_count() const { return locked_or_allocated_; }
    constexpr bool allocated() const { return locked_or_allocated_ != 3; }
};


#if FEATURE_STD_OSTREAM
template <class Char>
std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& out, block_mode_enum::modes m)
{
    return out << to_string(m);
}
#endif

}}

}}
