#pragma once

#include <estd/cstdint.h>
#include <estd/internal/macro/c++/ctor.h>
#include <estd/limits.h>
// DEBT: Make a span fwd
#include <estd/span.h>
#include <estd/utility.h>

#include "fwd.h"

// 17DEC25 MB - boilerplate for incoming playground.memory mem-11 formalization

namespace embr { namespace mem {

template <class T, int N>
struct container_traits<T[N]>
{
    static constexpr bool constexpr_size = true;

    static constexpr int size() { return N; }
    using value_type = T;
};

template <class T, int N>
struct container_traits<estd::span<T, N>>
{
    static constexpr bool constexpr_size = N != -1;

    static constexpr int size() { return N; }
    using value_type = T;
};

namespace detail { inline namespace v1 {

template <class Container>
struct handles_traits : container_traits<Container>
{
    using base_type = container_traits<Container>;
    using typename base_type::value_type;

    using size_type = uint8_t;
    using container_type = Container;

    static constexpr size_type unavailable = estd::numeric_limits<size_type>::max();

    static constexpr bool is_null(const value_type& v) { return v.is_null(); }
    static void reset(value_type& v) { v.reset(); }
};


template <class Traits>
class handles : public Traits
{
public:
    using traits = Traits;
    using typename traits::size_type;
    using typename traits::container_type;
    using typename traits::value_type;
    using traits::size;

protected:

    container_type container_;

public:
    ESTD_CPP_FORWARDING_CTOR_MEMBER(handles, container_)

    template <class F>
    size_type alloc(F&& on_alloc)
    {
        for(size_type i = 0; i < size(); ++i)
        {
            value_type& v = container_[i];

            if(traits::is_null(v))
            {
                on_alloc(v);
                return i;
            }
        }

        return traits::unavailable;
    }

    void dealloc(size_type handle)
    {
        assert(handle < size());

        traits::reset(container_[handle]);
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
