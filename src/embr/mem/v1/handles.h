#pragma once

#include <estd/cstdint.h>
#include <estd/internal/macro/c++/ctor.h>
#include <estd/limits.h>
#include <estd/system_error.h>
#include <estd/utility.h>


#include "bundle.h"
#include "filter-iterator.h"
#include "fwd.h"
#include "traits.h"


// 17DEC25 MB - boilerplate for incoming playground.memory mem-11 formalization

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

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
    using reference = value_type&;
    using const_reference = const value_type&;
    //using traits::size;

protected:

    container_type container_;

    struct filter
    {
        const value_type* end_;

        // TODO: Almost there, just need to compare end_ --
        constexpr bool operator()(const_reference v) const
        {
            if(&v == end_) return true;

            return traits::is_null(v) == false;
        }
    };

    constexpr filter pred() const { return { std::end(container_) }; }

public:
    using iterator = filter_iterator<filter, pointer>;
    using const_iterator = filter_iterator<filter, typename traits::const_pointer>;

    reference operator[](int i) { return container_[i]; }
    constexpr const_reference operator[](int i) const { return container_[i]; }

    ESTD_CPP_FORWARDING_CTOR_MEMBER(handles, container_)

    template <class P, class F>
    size_type alloc(P&& predicate, F&& on_alloc)
    {
        for(size_type i = 0; i < std::size(container_); ++i)
        {
            // Remember, we are allocating handles, not pool entries - so evaluating
            // is_null is correct for identifying unused handles
            if(reference v = container_[i]; traits::is_null(v) && predicate(i, v))
            {
                on_alloc(i, v);
                return i;
            }
        }

        return traits::null;
    }

    template <class F>
    size_type alloc(F&& on_alloc = [](int, reference) { return true; })
    {
        return alloc([](auto, auto) { return true; }, std::forward<F>(on_alloc));
    }

    estd::errc dealloc(size_type handle)
    {
        // DEBT: https://github.com/malachi-iot/estdlib/issues/166
        if(handle >= std::size(container_)) return estd::errc::bad_address;

        traits::reset(container_[handle]);

        return {};
    }

    void reset()
    {
        for(value_type& v : container_) traits::reset(v);
    }

    iterator begin() { return { pred(), &container_[0] }; }
    constexpr const_iterator begin() const { return { pred(), &container_[0] }; }
    iterator end() { return { pred(), &container_[std::size(container_)] }; }
    constexpr const_iterator end() const { return { pred(), &container_[std::size(container_)] }; }
};

}}

inline namespace v1 {


}


}}
