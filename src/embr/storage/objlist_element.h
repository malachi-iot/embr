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

template <int alignment, bool align_size = false, bool always_extra = false>
struct objlist_element
{
    using pointer = objlist_element*;

    // In bits
    static constexpr int alignment_ = alignment;

    friend class objlist_base<alignment, always_extra>;

private:
    unsigned size_ : 16;
    int next_ : 14;        ///< aligned pointer offset
    // Perhaps we can deduce this based on what list it is in?  Don't know.
    // It is convenient to have it here
    bool allocated_ : 1;
    bool moveptr_ : 1;      ///< First sizeof(intptr_t) bytes in data_ is a moveptr
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

    constexpr objlist_element(unsigned size, int next) :
        size_{size_shr(size)},
        next_{next},
        allocated_{false},
        moveptr_{false}
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
                (moveptr_ ? sizeof(objlist_element_extra) : 0) +
                sizeof(objlist_element);
    }

    constexpr int next_diff() const
    {
        return next_ << alignment_;
    }

    constexpr bool last() const
    {
        return next_ == 0;
    }

    objlist_element* next() const
    {
        if(last())  return nullptr;

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

    pointer last()
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
        if(moveptr_)
        {
            auto extra = (objlist_element_extra*)data_;

            extra->move_(this, nullptr);
        }
    }

    char* extended_data()
    {
        auto extra = (objlist_element_extra*) data_;
        return extra->data_;
    }

    char* data()
    {
        if(always_extra || moveptr_)
            return extended_data();
        else
            return data_;
    }

    template <class T, class ...Args>
    T* emplace(Args&&...args)
    {
        moveptr_ = true;
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
};


}}}
