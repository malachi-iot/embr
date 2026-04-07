#pragma once

#include <estd/memory.h>
//#include <estd/type_traits.h>

#include "../fwd.h"
#include "../mixins.h"

#include "fwd.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// Effectively alter ego of layer1::vector:
// - estd one is size + inline array + const length
// - this one is size + inline array + semi-const length
// Really similar mechanisms differing primarily in max_size acquisition and of course
// we have to lock here.  When adding inplace_vector https://github.com/malachi-iot/estdlib/issues/182
// consider consolidating this guy, if by then he's ready for estd'ness
template <class T, unsigned lock_bits>
class vector_control :
    public mixins::container<vector_control<T, lock_bits>, T>,
    public mixins::vector_erase<vector_control<T, lock_bits>, T>
{
    template <class T2, class Pool, Pool* pool>
    friend class mem::detail::v1::vector;

    template <class T2>
    friend class embr::mem::v1::pinned;

public:
    using size_type = unsigned;

    ESTD_CPP_STD_VALUE_TYPE(T)

    using iterator = pointer;
    using const_iterator = const_pointer;

    pointer data() { return reinterpret_cast<pointer>(this + 1); }
    const_pointer data() const { return reinterpret_cast<const_pointer>(this + 1); }

    constexpr vector_control() : size_{}, lock_count_{}        {};

    // uninitialized variants below important since they call placement new rather than
    // use operator=

    ESTD_CPP_CONSTEXPR(17) vector_control(const vector_control& copy_from) :
        size_{copy_from.size_},
        lock_count_{}
    {
        // DEBT: Make an estd flavor of this https://github.com/malachi-iot/estdlib/issues/181
        std::uninitialized_copy_n(copy_from.data(), size_, data());

        assert(copy_from.lock_count_ == 0);
    }

    // DEBT: If this guy isn't present, rtto incorrectly finds above copy_from during a move request
    ESTD_CPP_CONSTEXPR(17) vector_control(vector_control&& move_from) noexcept :
        size_{move_from.size_},
        lock_count_{}
    {
        // DEBT: Make an estd flavor of this https://github.com/malachi-iot/estdlib/issues/181
        std::uninitialized_move_n(move_from.data(), size_, data());

        assert(move_from.lock_count_ == 0);
    }

    // DEBT: See https://github.com/malachi-iot/estdlib/issues/185 as to whether WE should
    // be doing this
    ~vector_control()
    {
        const_pointer end = data() + size_;

        for(pointer begin = data(); begin < end; ++begin)
            begin->~value_type();
    }

private:
    static constexpr unsigned size_bits = sizeof(size_type) * 8 - lock_bits;

    size_type size_ : size_bits;

    // augmented lock counter for use with pinned_iterator
    // DEBT: Document why in particular this is more interesting than block->lock_count_ -
    //       IIRC it's to alleviate pressure on its bit space
    size_type lock_count_ : lock_bits;

public:

    constexpr size_type size() const { return size_; }

    // Limited version of resize, can only shrink
    void resize(size_type count)
    {
        assert(count <= size_);

        size_ = count;
    }
};


#if FEATURE_STD_TYPE_TRAITS
// Nope, variable size precludes this
//static_assert(std::is_trivially_move_constructible<vector_control<int>>::value);
#endif


}}

}}
