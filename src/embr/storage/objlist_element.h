#pragma once

#include "fwd.h"

namespace embr { namespace detail { inline namespace v1 {

typedef void (*objlist_element_move_fn)(void* source, void* dest);

struct objlist_element_extra
{
    objlist_element_move_fn move_;

    char data_[];
};

template <int alignment, bool align_size = false>
struct objlist_element
{
    // In bits
    static constexpr int alignment_ = alignment;

    friend class objlist_base<alignment>;

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

    objlist_element* next() const
    {
        if(next_ == 0)  return nullptr;

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

    void dealloc()
    {
        if(moveptr_)
        {
            auto extra = (objlist_element_extra*)data_;

            extra->move_(this, nullptr);
        }
    }

    char* data()
    {
        if(moveptr_)
            return data_ + sizeof(objlist_element_extra);
        else
            return data_;
    }
};


}}}
