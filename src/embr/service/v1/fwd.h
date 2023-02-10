#pragma once

#include "../../fwd/observer.h"

namespace embr {

inline namespace service { inline namespace v1 {

struct Service;

namespace host {

namespace internal {

}

template <class TImpl, class TSubject>
class ServiceBase2;

template <class TImpl, class TSubject>
class ServiceSparseBase;

template <class TImpl = v1::Service,
    class TSubject = embr::void_subject,
    class TBase = ServiceBase2<TImpl, TSubject> >
class Service;

}

template <class TImpl, class TSubject = embr::void_subject>
class ServiceSpec;

// DEBT: Experimenting, I think I like 'runtime' better than 'host'
namespace runtime = host;


}}

}
