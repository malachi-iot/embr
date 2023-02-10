#pragma once

#include <estd/type_traits.h>

namespace embr {

// Guidance from https://stackoverflow.com/questions/59984423/stdreference-wrapper-unwrap-the-wrapper
// DEBT: Consider putting this into estd, since it's such a companion to reference_wrapper
namespace detail {

template <typename type_t, class  orig_t>
struct unwrap_impl
{
    using type = orig_t;
    using is_wrapped = estd::false_type;
};

template <typename type_t, class V>
struct unwrap_impl<estd::reference_wrapper<type_t>,V>
{
    using type = type_t;
    using is_wrapped = estd::true_type;
};
}

template<class T>
struct unwrap
{
protected:
    typedef typename detail::unwrap_impl<estd::decay_t<T>, T> impl;

public:
    using type = typename impl::type;
    using is_wrapped = typename impl::is_wrapped;
};

template <typename type_t>
using unwrap_t = typename unwrap<type_t>::type;



}
