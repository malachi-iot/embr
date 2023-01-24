#pragma once

#include "../../fwd/observer.h"

namespace embr {

inline namespace service { inline namespace v1 {

struct Service;

namespace host {

template <class TImpl = v1::Service, class TSubject = embr::void_subject>
class Service;

}

template <class TImpl, class TSubject = embr::void_subject>
class ServiceSpec;


}}

}
