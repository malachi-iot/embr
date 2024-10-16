#pragma once

#include "fwd.h"

namespace embr { namespace detail { inline namespace v1 {

struct objlist_element_extra
{
    objlist_element_move_fn move_;

    template <class T>
    static void move_helper(void* source, void* dest)
    {
        auto s = (T*) source;
        T& s2 = *s;

        if(dest)
        {
            // FIX: Needs testing
            new (dest) T(std::move(s2));
        }

        s2.~T();
    }

    char data_[];
};

namespace internal {

struct objlist_element
{
};

}

template <int alignment, objlist_element_options options, class T2>
struct objlist_element : internal::objlist_element
{
    using pointer = objlist_element*;

    // In bits
    static constexpr int alignment_ = alignment;
    static constexpr bool align_size = options & OBJLIST_ELEMENT_ALIGN_SIZE;
    static constexpr bool always_extra = options & OBJLIST_ELEMENT_ALWAYS_EXTRA;
    static constexpr bool never_extra = false;

    friend class objlist_base<objlist_element>;

private:
    union
    {
        struct
        {
            unsigned size_ : 16;
            int next_ : 14;        ///< aligned pointer offset
            // Perhaps we can deduce this based on what list it is in?  Don't know.
            // It is convenient to have it here
            bool allocated_ : 1;
            bool extra_ : 1;      ///< First sizeof(intptr_t) bytes in data_ is a moveptr
        };

        char raw[1 << alignment_];
    };

    char data_[];

    static constexpr unsigned size_shl(unsigned sz)
    {
        return align_size ? sz << alignment : sz;
    }

    static constexpr unsigned size_shr(unsigned sz)
    {
        return align_size ? sz >> alignment : sz;
    }

public:
    constexpr objlist_element(unsigned size, int next, bool allocated) :
        size_{size_shr(size)},
        next_{next},
        allocated_{allocated},
        extra_{false}
    {

    }

    constexpr bool allocated() const
    {
        return allocated_;
    }

    // size of data payload
    constexpr unsigned size() const
    {
        return size_shl(size_);
    }

    constexpr unsigned total_size() const
    {
        // DEBT: This can be optimized and account for align_size
        return
            align<alignment>(size()) +
                (extra_ ? sizeof(objlist_element_extra) : 0) +
                sizeof(objlist_element);
    }

    constexpr int next_diff() const
    {
        return next_ << alignment_;
    }

    constexpr bool is_last() const
    {
        return next_ == 0;
    }

    objlist_element* next() const
    {
        if(is_last())  return nullptr;

        auto base = (char*) this;
        const int delta = next_diff();

        return reinterpret_cast<objlist_element*>(base + delta);
    }

    void next(objlist_element* v)
    {
        auto base = (char*) this;
        auto incoming = (char*) v;
        int delta = static_cast<int>(incoming - base);
        delta >>= alignment_;

        next_ = delta;
    }

    pointer get_last()
    {
        pointer p = this;

        for(;;)
        {
            pointer p_next = p->next();
            if(p_next == nullptr) return p;
            p = p_next;
        }

        // Looks like I finally discovered the use of a non-returning returning function
        return {};
    }

    void dealloc()
    {
        if(extra_)
        {
            auto extra = (objlist_element_extra*)data_;

            extra->move_(this, nullptr);
        }
    }

    T2* extended_data()
    {
        auto extra = (objlist_element_extra*) data_;
        return static_cast<T2*>(extra->data_);
    }

    char* data()
    {
        if(always_extra || extra_)
            return extended_data();
        else
            return data_;
    }

    template <class T, class ...Args>
    T* emplace_without_extra(Args&&...args)
    {
        return new (data_) T(std::forward<Args>(args)...);
    }

    template <class T, class ...Args>
    T* emplace(Args&&...args)
    {
        if constexpr(never_extra) return emplace_without_extra<T>(std::forward<Args>(args)...);

        extra_ = true;
        auto data = (objlist_element_extra*) data_;
        data->move_ = objlist_element_extra::move_helper<T>;
        return new (data->data_) T(std::forward<Args>(args)...);
    }

    /// From this node, walk forward to the end of the node chain
    /// @tparam F
    /// @param f
    // DEBT: A const flavor of this would be very nice
    template <class F>
    void walk(F&& f)
    {
        pointer current = this;

        // NOTE: A little crazy this works with a null this, but why not...
        while(current != nullptr)
        {
            pointer next = current->next();

            f(current, next);

            current = next;
        }
    }

    // EXPERIMENTAL
    operator objlist_element<alignment, options, char*>&()
    {
        auto c = (objlist_element<alignment, options, char*>*) this;

        return *c;
    }
};


}}}
