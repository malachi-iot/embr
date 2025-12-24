#pragma once

#include <estd/internal/rtto.h>

#include "enum.h"
#include "fwd.h"
#include "traits.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

struct block_mode_base : block_mode_enum
{
    // DEBT: Do stricter check to make sure prev_/next_ are compatible
    static constexpr unsigned null = handles_traits_base::null;
    using handle_type = handles_traits_base::size_type;
};

class alignas(void*) block : public block_mode_base
{
    // DEBT: rtto base is WAY overloaded.  Needs attention
    using rtto_base_type = estd::internal::rtto_base::base;
    using rtto_proxy = estd::internal::rtto_base::rtto_base::proxy<>;

    template <class T>
    using rtto = estd::internal::rtto<T>;

    template <class HandlesTraits, class Page>
    friend class bundle_base;

protected:
    struct alignas(void*)
    {
        unsigned prev_ : 8;
        unsigned next_ : 8;
        modes mode_ : 2;
        bool allocated_ : 1;
        unsigned lock_count_ : 4;
        unsigned ref_count_ : 4;
    };

    char data_[];

public:
    block() = default;
    explicit block(modes mode, bool allocated,
        unsigned prev = null, unsigned next = null) :
        prev_{prev},
        next_{next},
        mode_{mode},
        allocated_{allocated},
        lock_count_{0},
        ref_count_{0}
    {}

    // emplace constructors
    // DEBT: Nifty idea, but unfortunately flawed.  By the time we reach a block to do this, it's
    // already a free-block so technically performing a 2nd in-place construction is bad form.
    // That's despite the fact that it's safe and functional.
    template <class T, class ...Args>
    explicit block(estd::in_place_index_t<modes::Immobile>, estd::in_place_type_t<T>, Args&&...args);

    template <class T, class ...Args>
    explicit block(estd::in_place_index_t<modes::Trivial>, estd::in_place_type_t<T>, Args&&...args);

    template <class T, class ...Args>
    explicit block(estd::in_place_index_t<modes::RttoProxy>, estd::in_place_type_t<T>, Args&&...args);

    template <class T, class ...Args>
    explicit block(estd::in_place_index_t<modes::RttoBase>, estd::in_place_type_t<T>, Args&&...args);

    block& operator=(const block&) = default;
    block& operator=(block&&);

    constexpr modes mode() const { return mode_; }
    constexpr handle_type prev() const { return prev_; }
    constexpr handle_type next() const { return next_; }
    constexpr bool allocated() const { return allocated_; }
    constexpr unsigned lock_count() const { return lock_count_; }

    // TBD
    constexpr bool invariant() const
    {
        // Circular linked list not allowed
        return (prev_ == null && next_ == null) || prev_ != next_;
    }

    void* data() { return data_; }
    rtto_proxy* proxy() { return (rtto_proxy*) data_; }
    rtto_base_type* rtto_base() { return (rtto_base_type*) data_; }

    // FIX: Probably not 100% right, because RttoBase mode includes size of rtto::u_ in the object itself
    template <modes mode>
    static constexpr unsigned header_size()
    {
        return mode == Trivial ? sizeof(block) : (sizeof(block) + sizeof(estd::internal::rtto_base::base));
    }

    // DEBT: Protect this and make friend classes, or pull WriteableBlock child stunt
    void allocated(bool v)      { allocated_ = v; }
    void mode(modes v)          { mode_ = v; }
    void reset(modes mode, bool allocated)
    {
        mode_ = mode;
        allocated_ = allocated;
        lock_count_ = 0;
        ref_count_ = 0;
    }

    template <class T, class ...Args>
    void emplace_rtto_proxied(Args&&...args);

    template <class T, class ...Args>
    void emplace(Args&&...args);

    // Destroy tracked object, not necessarily block itself
    void destroy();

    void move_from(block* from, unsigned sz);
};

#if UNIT_TESTING
/*
struct block_exposer : block
{
    using block::data_;
};

static_assert(offsetof(block_exposer, data_) == sizeof(void*));*/
#endif


// EXPERIMENTAL
class alignas(void*) small_block : block_mode_base
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
