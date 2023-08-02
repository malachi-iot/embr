#pragma once

#include "service.h"

namespace embr {

inline namespace service { inline namespace v1 {

struct SparseService : embr::PropertyContainer,
    ServiceBase
{
    typedef SparseService this_type;

    static constexpr const char* name() { return "Generic sparse service"; }

    using id = Service::id;

    template <class TSubject, class TImpl = this_type>
    using runtime = embr::service::v1::runtime::Service<TImpl, TSubject,
        embr::service::v1::runtime::internal::SparseServiceBase<TImpl, TSubject> >;

    // dummy, we never record state at all
    template <class S>
    void state(S)   {}

    // sparse services are always default to the Started, Running state

    static constexpr States state()         { return Started; }
    static constexpr Substates substate()   { return Running; }
};

namespace host {

namespace internal {

// NOTE: No 'changing' events eminate since we don't specifically track previous state to know
// whether it changed or not.  'changed' itself is trusted to be true as initiated by implementing
// service.  Furthermore, 'changed' event naturally mates to stateful service, so we must pass in a
// null owner here since no owner exists to host the state variables.
// DEBT: Name and namespace for this need work
template <class TImpl, class TSubject>
class SparseServiceBase : public PropertyHost<TImpl, TSubject>
{
    typedef PropertyHost<TImpl, TSubject> base_type;

    using st = v1::Service;

protected:
    ESTD_CPP_FORWARDING_CTOR(SparseServiceBase)

    void state(st::States v)
    {
        base_type::template fire_changed_null_owner<st::id::state>(v);
    }

    void state(st::Substates v)
    {
        base_type::template fire_changed_null_owner<st::id::substate>(v);
    }

    void state(st::States v, st::Substates ss)
    {
        // NOTE: This deviates from regular stateful service which only fires one or the other,
        // not both
        state(ss);
        state(v);
    }
};


}   // internal

}


}}

}
