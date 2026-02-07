#pragma once

#include <estd/cstdint.h>
#include <estd/internal/macro/c++/ctor.h>
#include <estd/limits.h>
#include <estd/system_error.h>
#include <estd/utility.h>


#include "filter-iterator.h"
#include "error.h"
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

    // Filter by not-null elements and always include end element for consumer
    // comparison
    struct filter
    {
        const value_type* end_;

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
        for(size_type i = 0; i < estd::size(container_); ++i)
        {
            reference v = container_[i];

            // Remember, we are allocating handles, not pool entries - so evaluating
            // is_null is correct for identifying unused handles
            if(traits::is_null(v) && predicate(i, v))
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
        if(handle >= estd::size(container_)) return estd::errc::bad_address;

        traits::reset(container_[handle]);

        return {};
    }

    ESTD_CPP_CONSTEXPR(14) void reset()
    {
        for(value_type& v : container_) traits::reset(v);
    }

    iterator begin() { return { pred(), &container_[0] }; }
    constexpr const_iterator begin() const { return { pred(), &container_[0] }; }
    iterator end() { return { pred(), &container_[estd::size(container_)] }; }
    constexpr const_iterator end() const { return { pred(), &container_[estd::size(container_)] }; }

    constexpr unsigned size() const { return estd::size(container_); }

    const value_type* first_zero() const
    {
        for(const_reference page : container_)  if(traits::is_zero(page)) return &page;

        return nullptr;
    }

    // DEBT: Not 100% sure we want zero-pos to qualify as an invariant check
    invariant_result invariant() const
    {
        using result = invariant_result::unexpected_type;

        int valid = 0;

        for(const_reference page : container_)  if(traits::is_zero(page))   ++valid;

        if(valid != 1)  return result({"zero check failed", "phase 1"});

        return {};
    }
};

}}

inline namespace v1 {


}


}}
