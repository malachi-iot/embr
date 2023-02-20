#pragma once

#include "../../fwd/observer.h"
#include "../../property/v1/fwd.h"

namespace embr {

inline namespace service { inline namespace v1 {

struct Service;

namespace host {

namespace internal {

template <class TImpl, class TSubject>
class ServiceBase;

template <class TImpl, class TSubject>
class SparseServiceBase;

}

// We utilize TBase to foster switching between regular and sparse service
template <class TImpl = v1::Service,
    class TSubject = embr::void_subject,
    class TBase = internal::ServiceBase<TImpl, TSubject> >
class Service;

}

template <class TImpl, class TSubject = embr::void_subject>
class ServiceSpec;

// DEBT: Experimenting, I think I like 'runtime' better than 'host'
namespace runtime = host;


}}

}
