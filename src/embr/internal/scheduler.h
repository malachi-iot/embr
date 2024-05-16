#pragma once

#include "../observer.h"
#include "scoped_lock.h"
#include "impl/scheduler.h"

namespace embr {

namespace internal {


namespace scheduler {

// DEBT: See Scheduler constructor comments
struct impl_params_tag {};

}


namespace events {

template<class Impl>
struct Scheduler
{
    typedef Impl impl_type;
};


// DEBT: Poor name for schedule-specific event struct
template<class Impl, bool is_const = true>
struct ValueBase : Scheduler<Impl>
{
    typedef typename Impl::value_type _value_type;
    typedef typename estd::conditional<is_const, const _value_type, _value_type>::type value_type;

    // DEBT: Needs better name, this represents the control/meta structure going in
    // the sorted heap.  Be advised the heap one may be a copy of this
    value_type& value;

    ValueBase(value_type& value) : value(value) {}
};

template <class Impl>
struct Scheduling : ValueBase<Impl>
{
    typedef ValueBase<Impl> base_type;

    Scheduling(typename base_type::value_type& value) : base_type(value) {}
};


// DEBT: Consider how to semi standardize collection operation events, somewhat similar to how C#
// does with IObservableCollection
template <class Impl>
struct Scheduled : ValueBase<Impl>
{
    typedef ValueBase<Impl> base_type;

    Scheduled(typename base_type::value_type& value) : base_type(value) {}
};


template <class Impl>
struct Removed : ValueBase<Impl>
{
    typedef ValueBase<Impl> base_type;

    Removed(typename base_type::value_type& value) : base_type(value) {}
};


template <class Impl>
struct Processing : ValueBase<Impl>
{
    typedef ValueBase<Impl> base_type;
    typedef typename base_type::impl_type::time_point time_point;

    const time_point current_time;

    Processing(typename base_type::value_type& value, time_point current_time) :
        base_type(value), current_time(current_time)
    {}
};


template <class Impl>
struct Processed : Scheduler<Impl>
{
    typedef Scheduler<Impl> base_type;
    typedef typename base_type::impl_type::time_point time_point;

    const time_point current_time;

    // DEBT: Need to actually store 'value'

    Processed(typename base_type::impl_type::value_type* value, time_point current_time) :
        current_time(current_time)
    {}
};

}

}

}