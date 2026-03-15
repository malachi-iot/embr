#pragma once

#include <estd/internal/dynamic_array.h>
#include <estd/memory.h>

#if FEATURE_STD_OSTREAM
#include <estd/iosfwd.h>
#endif

#include "block.h"
#include "fwd.h"
#include "pool/construct.hpp"
#include "unit.h"


#define USE_REAL_HANDLE_OFFSET  1
#define EMBR_VECTOR_ADV_ACCESSOR 0

namespace embr { namespace mem {

inline namespace v1 {

template <class T, class Pool, Pool* pool = nullptr>
using vector = estd::internal::dynamic_array<mem::v1::vector_impl<T, Pool, pool>>;

}

namespace detail { inline namespace v1 {


// Effectively alter ego of layer1::vector:
// - estd one is size + inline array + const length
// - this one is size + inline array + semi-const length
// Really similar mechanisms differing primarily in max_size acquisition and of course
// we have to lock here.  When adding inplace_vector https://github.com/malachi-iot/estdlib/issues/182
// consider consolidating this guy, if by then he's ready for estd'ness
template <class T, unsigned lock_bits = 3>
class vector_impl
{
    template <class T2, class Pool, Pool* pool>
    friend class mem::v1::vector_impl;

    template <class T2>
    friend class embr::mem::v1::pinned;

public:
    using size_type = unsigned;

    ESTD_CPP_STD_VALUE_TYPE(T)

    pointer data() { return reinterpret_cast<pointer>(this + 1); }
    const_pointer data() const { return reinterpret_cast<const_pointer>(this + 1); }

    constexpr vector_impl() : size_{}, lock_count_{}        {};

    // uninitialized variants below important since they call placement new rather than
    // use operator=

    vector_impl(const vector_impl& copy_from) :
        size_{copy_from.size_}
    {
        // DEBT: Make an estd flavor of this https://github.com/malachi-iot/estdlib/issues/181
        std::uninitialized_copy_n(copy_from.data(), size_, data());
    }

    // DEBT: If this guy isn't present, rtto incorrectly finds above copy_from during a move request
    vector_impl(vector_impl&& move_from) :
        size_{move_from.size_}
    {
        // DEBT: Make an estd flavor of this https://github.com/malachi-iot/estdlib/issues/181
        std::uninitialized_move_n(move_from.data(), size_, data());
    }

    // DEBT: See https://github.com/malachi-iot/estdlib/issues/185 as to whether WE should
    // be doing this
    ~vector_impl()
    {
        const_pointer end = data() + size_;

        for(pointer begin = data(); begin < end; ++begin)
            begin->~value_type();
    }

private:
    static constexpr unsigned size_bits = sizeof(size_type) * 8 - lock_bits;

    size_type size_ : size_bits;
    size_type lock_count_ : lock_bits;

public:

    constexpr size_type size() const { return size_; }
};


}}

inline namespace v1 {

// Heavy lift
// Not done, I'd say over the hump for a proof of concept
template <class T, class Pool, Pool* pool>
class vector_impl : public mem::v1::unique_handle<detail::vector_impl<T>, Pool, pool>
{
    using control_type = detail::vector_impl<T>;
    using base_type = mem::v1::unique_handle<control_type, Pool, pool>;
    using this_type = vector_impl;
    using typename base_type::ops_type;
    // DEBT: disambiguate lock_handle and handle_type, causing confusion
    using typename base_type::handle_type;
    using pos_type = typename ops_type::pos_type;
    using bundle = typename ops_type::bundle;
    using const_bundle = typename ops_type::const_bundle;
    using base_type::ops;
    using base_type::guard;
    using bytes = estd::units::bytes<unsigned>;

    using base_type::handle_;

    static constexpr unsigned control_size = sizeof(control_type);

    //static_assert(control_size == sizeof(void*));

    template <class ...Args>
    bundle construct_ll(int reserved, Args&&...args)
    {
        using block = embr::mem::detail::block_8;

        constexpr block::modes mode = embr::mem::detail::ascertain_block_mode<control_type>();
        constexpr bytes block_sz = block::header_size(mode);
        unsigned sz = reserved * sizeof(T) + block_sz.count() + sizeof(control_type);

        // construct_ll takes explicit size as 1st parameter as you might glean
        return ops().template construct_ll<mode, control_type>(ops().do_alias(sz), std::forward<Args>(args)...);
    }

    // Low-level end() - does NOT do locking, so be careful
    const T* end_ll() const
    {
        const control_type* control = base_type::data();
        return control->data() + control->size_;
    }

public:
    using base_type::pool_;

    vector_impl(Pool* p) : base_type(base_type::null, p)  {}

    vector_impl(const vector_impl& copy_from) :
        base_type(base_type::null, copy_from.pool_())
    {
        if(!copy_from.has_value())  return;

        const control_type* c = copy_from.clock();

        // DEBT: Consider overprovisioning
        handle_ = construct_ll(c->size_, *c).handle;

        copy_from.unlock();
    }

    vector_impl(vector_impl&& move_from) :
        base_type(move_from.handle_, move_from.pool_())
    {
        move_from.handle_ = base_type::null;
    }

    using size_type = unsigned;

    ESTD_CPP_STD_VALUE_TYPE(T)

    struct policy_type
    {

    };

#if USE_REAL_HANDLE_OFFSET
    using handle_with_offset = estd::internal::handle_with_offset<handle_type>;
#else
    using handle_with_offset = estd::internal::handle_with_offset_raw<pointer>;
#endif

    struct allocator_type
    {
        using size_type = unsigned;

        ESTD_CPP_STD_VALUE_TYPE(T)

        Pool* pool_;

        // DEBT: Document why we need this up in estd
        using const_void_pointer = const void*;
        using handle_with_offset = typename this_type::handle_with_offset;

        reference lock(const handle_with_offset& h) { return *h.handle(); }
    };

    struct allocator_traits
    {
        ESTD_CPP_STD_VALUE_TYPE(T)
        using size_type = unsigned;
        using handle_type = mem::detail::v1::lock_handle<Pool, pool>;

        // FIX: One or multiple of these are wanting to be an accessor
        using allocator_valref = allocator_type;
        //using iterator = pointer;
        using const_iterator = const_pointer;
#if USE_REAL_HANDLE_OFFSET
#if EMBR_VECTOR_ADV_ACCESSOR
        struct accessor : handle_type
        {
            using base_type = handle_type;

            int offset_;
            using locked_type = reference;
            using const_locked_type = const_reference;

            accessor(allocator_type allocator, handle_with_offset hwo) :
                base_type(hwo.handle(), allocator.pool_),
                offset_{int(hwo.offset())}
            {

            }

            constexpr bool operator==(const_reference compare_to) const
            {
                return false;
            }

            // FIX:
            const_reference clock() const { static value_type dummy; return dummy; }
            void cunlock() { }
            const_reference value() const { static value_type dummy; return dummy ;}
        };
#else
        struct accessor_impl :
            handle_type
            //mem::detail::mixin::typed_handle<accessor_impl, value_type>
        {
            int offset_;

            using base_type = handle_type;
            //using mixin_type = mem::detail::mixin::typed_handle<accessor_impl, value_type>;
            //using mixin_type::lock;

            ESTD_CPP_STD_VALUE_TYPE(value_type)

            //ESTD_CPP_FORWARDING_CTOR(accessor_impl);

            accessor_impl(allocator_type allocator, handle_with_offset hwo) :
                base_type(hwo.handle(), allocator.pool_),
                offset_{int(hwo.offset())}
            {

            }

            // Dormant - since this is hidden inside 'impl'.  mixin/crtp accessors would probably help
            lock_guard<T, Pool, pool> guard()
            {
                return { *this };
            }

            using offset_type = int;
            using const_offset_type = int;
            using locked_type = reference;
            using const_locked_type = const_reference;

            // DEBT: Continued awkwardness with lock returning ref
            // See https://github.com/malachi-iot/estdlib/issues/88
            locked_type lock() const
            {
                auto control = (control_type*)base_type::lock();
                return *(control->data() + offset_);
            }
        };

        // Needs to be typed to T
        // TODO: Rework this to be 100% custom accessor all our own with improved features:
        // 1. always operating like a lock_guard
        // 2. have an accessor something like value_type&& value() and/or operator value_type&&()
        //    so that it's enforced that the locked data doesn't outlive the accessor
        // 3. have a disambiguated flavor of above accessor that really does return value_type&
        //    just make it as clear as possible THAT guy in danger of invalidating quickly
        // 4. lock/unlock theoretically not necessary or wanted anymore since it's auto locked
        using accessor = estd::internal::locking_accessor<accessor_impl>;
#endif
#else
        using accessor = estd::internal::traditional_accessor<value_type>;
#endif
        using iterator = estd::internal::locking_iterator<allocator_type, accessor>;
        using handle_with_offset = typename this_type::handle_with_offset;

        static constexpr auto locking_preference = estd::internal::allocator_locking_preference::standard;
    };

    handle_with_offset offset(unsigned pos) const
    {
        // FIX: We need the non-pointer variety of handle_with_offset
        // FIX: Feed this non-nullptr
        // FIX: We need to not use this temporal pointer
        control_type* control = base_type::lock();
        base_type::unlock();

#if USE_REAL_HANDLE_OFFSET
        return { handle_, pos };
#else
        return { control->data() + pos };
#endif
    }

    ESTD_CPP_CONSTEXPR(17) reference lock(unsigned pos = 0, unsigned count = 0)
    {
        control_type* control = base_type::lock();

        // DEBT: lock return a reference is somewhat counterintuitive
        return *(control->data() + pos);
    }

    size_type size() const
    {
        if(!is_allocated()) return 0;

        return guard()->size_;
    }

    size_type max_size() const
    {
        // This involves asking pool/ops for available contiguous free space.  Don't do fancy
        // theoretical/defrag calc here - let someone else do defragging first before calling max_size()
        return {};  // TBD
    }

    unsigned capacity() const
    {
        if(is_allocated() == false) return 0;

        // DEBT: assert (in debug mode only) that we're evenly divisible
        const bytes size = base_type::logical_size();
        return (size.count() - control_size) / sizeof(T);
    }

    constexpr bool is_allocated() const { return base_type::has_value(); }

    void size(unsigned new_size)
    {
        /*
        bytes rsz(new_size * sizeof(T));
        bytes sz = ops().phys_size(get_bundle());

        sz -= control_size;

        // DEBT: Consider padding here
        if(rsz > sz)    assert(reallocate(new_size));   */
        guard()->size_ = new_size;
    }

    bool reallocate(unsigned capacity)
    {
        // DEBT: Check estd, it may be that reallocate is NEVER called in this unallocated
        // condition.  Leaning strongly towards it handles that for us
        if(is_allocated() == false) return allocate(capacity);

        bool success = pool_()->realloc(handle_, control_size + capacity * sizeof(T), &handle_);
        return success;
    }

    allocator_type get_allocator() { return { pool_() }; }

    bool allocate(unsigned capacity)
    {
        assert(!is_allocated());

        handle_ = construct_ll(capacity).handle;

        return is_allocated();
    }

    // DEBT: Seems superfluous - like estd could do all this on our behalf
    template <class ...Args>
    void construct(int pos, Args&&...args)
    {
        control_type* control = base_type::lock();

        new (control->data() + pos) value_type(std::forward<Args>(args)...);

        base_type::unlock();
    }

    // TODO: Do sentry comparison

    class sentinel;

    // EXPERIMENTAL
    class pinned_iterator : public mixins::iterator<pinned_iterator, value_type>
    {
        friend sentinel;
        friend this_type;

        this_type* parent_;
        pointer current_;

        template <class Derived, class T2>
        friend class mixins::iterator_access;

        template <class Derived>
        friend class mixins::iterator_math;

        constexpr pinned_iterator(this_type* parent, pointer current) :
            parent_{parent}, current_{current}
        {}

    public:
        constexpr pinned_iterator(const pinned_iterator& copy_from) :
            parent_{copy_from.parent_}, current_{copy_from.current_}
        {
            control_type* control = parent_->data();

            ++control->lock_count_;
        }

        ~pinned_iterator()
        {
            control_type* control = parent_->data();

            if(--control->lock_count_ == 0)
            {
                parent_->unlock();
            }
        }
    };


    // EXPERIMENTAL
    class sentinel
    {
        this_type* parent_;

        pinned_iterator promote()
        {
            return { parent_, parent_->end_ll() };
        }

    public:
        sentinel(this_type* parent) : parent_{parent}   {}

        pinned_iterator operator--(int)
        {
            return --promote();
        }

        operator pinned_iterator() { return promote(); }

        friend constexpr bool operator==(sentinel lhs, const pinned_iterator& rhs)
        {
            return lhs.parent_->end_ll() == rhs.current_;
        }

        friend constexpr bool operator==(const pinned_iterator& lhs, sentinel rhs)
        {
            return rhs.parent_->end_ll() == lhs.current_;
        }

        friend constexpr bool operator!=(const pinned_iterator& lhs, sentinel rhs)
        {
            return rhs.parent_->end_ll() != lhs.current_;
        }
    };

    // EXPERIMENTAL
    // probably will work in which case change this to begin() and if it works really well, support this up
    // in estd
    pinned_iterator pinned_begin()
    {
        control_type* control = base_type::lock();
        ++control->lock_count_;

        return { this, control->data() };
    }

    // EXPERIMENTAL
    sentinel pinned_end()
    {
        return sentinel{this};
    }
};

#if FEATURE_STD_OSTREAM && EMBR_VECTOR_ADV_ACCESSOR
// FIX: ADL doesn't seem to pick this up, perhaps because they are inner classes?
template <class Char, class T, class Pool, Pool* pool>
std::basic_ostream<Char>& operator <<(std::basic_ostream<Char>& out,
    const typename vector_impl<T, Pool, pool>::allocator_traits::accessor& acc)
{
    return out << acc.value();
}

#endif

}   // v1

}}  // embr::mem
