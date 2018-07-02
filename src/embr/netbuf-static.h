#pragma once

#include <estd/span.h>
#include <estd/array.h>
#include <estd/vector.h>

namespace embr { namespace mem {

enum ExpandResult
{
    ExpandFailFixedSize = -2,
    ExpandFailOutOfMemory = -1,
    ExpandOKChained = 0,
    ExpandOKLinear = 1
};

namespace layer1 {

template <size_t N>
class NetBuf : public estd::array<uint8_t, N>
{
    typedef estd::array<uint8_t, N> base;

public:
    typedef typename base::value_type value_type;

#ifdef FEATURE_CPP_INITIALIZER_LIST
    NetBuf(::std::initializer_list<value_type> init) : base(init)
    {

    }

    NetBuf() = default;
#endif

    typedef typename base::size_type size_type;

    bool next() const { return false; }

    ExpandResult expand(size_type by_amount)
    {
        return ExpandResult::ExpandFailFixedSize;
    }

    bool last() const { return true; }

    size_type total_size() { return N; }
};

}

namespace layer2 {

template <size_t N>
class NetBuf : public estd::layer1::vector<uint8_t, N>
{
    typedef estd::layer1::vector<uint8_t, N> base;

public:
    typedef typename base::value_type value_type;

#ifdef FEATURE_CPP_INITIALIZER_LIST
    NetBuf(::std::initializer_list<value_type> init) : base(init)
    {

    }

    NetBuf() = default;
#endif

    typedef typename base::size_type size_type;

    bool next() const { return false; }

    ExpandResult expand(size_type by_amount)
    {
        if(base::resize(base::size() + by_amount))
            return ExpandResult::ExpandOKLinear;
        else
            return ExpandResult::ExpandFailOutOfMemory;
    }

    bool last() const { return true; }

    size_type total_size() { return base::size(); }

    // NOTE: Mild abuse of clock, but we know that layer1 vectors are always
    // simple buffers so not too bad.  Indicates that really we should put a data()
    // into those containers which are gaurunteed to be that simple
    const uint8_t* data() const { return base::clock(); }
};

}

}}
