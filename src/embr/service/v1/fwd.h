#pragma once

#include "../../fwd/observer.h"

namespace embr {

namespace service { inline namespace v1 {

namespace impl {

struct Service;

}

template <class TImpl = impl::Service, class TSubject = embr::void_subject>
class Service;

template <class TImpl, class TSubject = embr::void_subject>
class ServiceSpec;


}}

}