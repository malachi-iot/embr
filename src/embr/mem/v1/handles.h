#pragma once

#include <estd/cstdint.h>
#include <estd/internal/macro/c++/ctor.h>
#include <estd/limits.h>
// DEBT: Make a span fwd
#include <estd/span.h>
#include <estd/system_error.h>
#include <estd/utility.h>


#include "bundle.h"
#include "filter-iterator.h"
#include "fwd.h"


// 17DEC25 MB - boilerplate for incoming playground.memory mem-11 formalization

namespace embr { namespace mem {

// DEBT: container_traits looking pretty useful.  Consider putting him up at estd level
template <class T, int N>
struct container_traits<T[N]>
{
    static constexpr bool constexpr_size = true;

    static constexpr int size() { return N; }

    using container_type = T[N];

    ESTD_CPP_STD_VALUE_TYPE(T)

    static T* data(container_type& c) { return c; }
};

template <class T, estd::size_t N>
struct container_traits<estd::span<T, N>>
{
    static constexpr bool constexpr_size = N != -1;

    static constexpr int size() { return N; }

    using container_type = estd::span<T, N>;

    ESTD_CPP_STD_VALUE_TYPE(T)

    static pointer data(container_type& c) { return c.data(); }
};

namespace detail { inline namespace v1 {

template <class Container>
struct handles_traits : container_traits<Container>
{
    using base_type = container_traits<Container>;
    using typename base_type::value_type;

    using size_type = uint8_t;

    static constexpr size_type unavailable = estd::numeric_limits<size_type>::max();

    static constexpr bool is_null(const value_type& v) { return v.is_null(); }
    static void reset(value_type& v) { v.reset(); }

    struct bundle
    {
        value_type* value;
        size_type handle;
    };
};

template <class Traits>
class handles : public Traits
{
    using this_type = handles;

public:
    using traits = Traits;
    using typename traits::size_type;
    using typename traits::container_type;
    using typename traits::value_type;
    using typename traits::pointer;
    using traits::size;

protected:

    container_type container_;

public:
    class iterator
    {
        pointer current_;

    public:

    };

    value_type& operator[](int i) { return container_[i]; }

    ESTD_CPP_FORWARDING_CTOR_MEMBER(handles, container_)

    template <class P, class F>
    size_type alloc(P&& predicate, F&& on_alloc)
    {
        for(size_type i = 0; i < size(); ++i)
        {
            value_type& v = container_[i];

            // FIX: Need this as is_null == false, since that is NOT the same as allocated
            if(traits::is_null(v) && predicate(i, v))
            {
                on_alloc(i, v);
                return i;
            }
        }

        return traits::unavailable;
    }

    estd::errc dealloc(size_type handle)
    {
        if(handle >= size()) return estd::errc::bad_address;

        traits::reset(container_[handle]);

        return {};
    }

    void reset()
    {
        for(value_type& v : container_) traits::reset(v);
    }
};

}}

inline namespace v1 {


}


}}
