#pragma once

#include <estd/cstdint.h>
#include <estd/internal/macro/c++/ctor.h>
#include <estd/utility.h>

#include "fwd.h"
#include "block.h"
#include "handles.h"
#include "page.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// 16DEC25 MB - boilerplate for incoming playground.memory mem-11 formalization

template <class Container, class Traits>
class pool
{
protected:
    using container_type = Container;

    container_type pool_;

public:
    ESTD_CPP_FORWARDING_CTOR_MEMBER(pool, pool_)

    using traits = Traits;
    using handle_type = int;

    handle_type alloc(unsigned logical_sz, unsigned block_sz);
    void dealloc(handle_type);
};

}}

inline namespace v1 {


}

}}
