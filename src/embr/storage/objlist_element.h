#pragma once

namespace embr { namespace detail { inline namespace v1 {

template <int alignment, bool align_size = false>
struct objlist_element
{
    // In bits
    static constexpr int alignment_ = alignment;

private:
    ///< true unaligned size
    unsigned size_ : 16;

public:
    int next_ : 14;    ///< aligned pointer offset

    // Perhaps we can deduce this based on what list it is in?  Don't know.
    // It is convenient to have it here
    bool allocated_ : 1;

    static constexpr unsigned size_shl(unsigned sz)
    {
        return align_size ? sz << alignment : sz;
    }

    static constexpr unsigned size_shr(unsigned sz)
    {
        return align_size ? sz >> alignment : sz;
    }

    constexpr objlist_element(unsigned size, int next) :
        size_{size_shr(size)},
        next_{next},
        allocated_{false}
    {

    }

    constexpr unsigned size() const
    {
        return size_shl(size_);
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

    char data_[];
};


}}}
