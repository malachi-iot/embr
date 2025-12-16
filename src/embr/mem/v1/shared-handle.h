#pragma once

#include "fwd.h"

namespace embr { namespace mem { inline namespace v1 {

template <class T, class Pool, Pool* pool>
class shared_handle : public detail::shared_handle<Pool, pool>
{
    using base_type = detail::shared_handle<Pool, pool>;
};

}}}
