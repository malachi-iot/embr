#pragma once

#include <estd/span.h>
#include <estd/array.h>
#include <estd/vector.h>

#include "netbuf.h"

namespace embr { namespace mem {

namespace layer1 {

// private inheritance so that we don't collide on oft-used methods like last, end, data(), etc
template <size_t N>
class NetBuf : estd::array<uint8_t, N>
{
    typedef estd::array<uint8_t, N> base;

public:
    typedef typename base::value_type value_type;
    typedef typename base::size_type size_type;

#ifdef FEATURE_CPP_INITIALIZER_LIST
    NetBuf(::std::initializer_list<value_type> init) : base(init)
    {

    }

    NetBuf() = default;
#endif

    bool next() const { return false; }

    ExpandResult expand(size_type by_amount, bool move_to_next)
    {
        return ExpandResult::ExpandFailFixedSize;
    }

    bool last() const { return true; }

    size_type total_size() const { return N; }
    size_type size() const { return base::size(); }

    const uint8_t* data() const { return base::data(); }

    uint8_t* data() { return base::data(); }

    // position back at the beginning
    void reset() {}
};

}

namespace layer2 {

template <size_t N>
class NetBuf : estd::layer1::vector<uint8_t, N>
{
    typedef estd::layer1::vector<uint8_t, N> base;

public:
    typedef typename base::value_type value_type;
    typedef typename base::size_type size_type;

#ifdef FEATURE_CPP_INITIALIZER_LIST
    NetBuf(::std::initializer_list<value_type> init) : base(init)
    {

    }

    NetBuf() = default;
#endif

    bool next() const { return false; }

    ExpandResult expand(size_type by_amount, bool move_to_next)
    {
        if(base::resize(base::size() + by_amount))
            return ExpandResult::ExpandOKLinear;
        else
            return ExpandResult::ExpandFailOutOfMemory;
    }

    // shrink to a specified amount.  note our shrink differs from
    // PBUF shrink in that it only applies to current data()
    bool shrink_experimental(size_type to_amount)
    {
        return base::resize(to_amount);
    }

    bool last() const { return true; }

    size_type size() const { return base::size(); }
    size_type total_size() const { return base::size(); }

    const uint8_t* data() const { return base::data(); }

    uint8_t* data() { return base::data(); }

    // position back at the beginning
    void reset() {}
};

}

template <size_t N>
struct NetBufTraits<layer1::NetBuf<N> >
{
    static CONSTEXPR bool can_chain() { return false; }
    static CONSTEXPR bool can_expand() { return false; }
};

template <size_t N>
struct NetBufTraits<layer2::NetBuf<N> >
{
    static CONSTEXPR bool can_chain() { return false; }
    static CONSTEXPR bool can_expand() { return true; }
};


}}
