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

template <class Container>
struct pool_traits : container_traits<Container>
{

};

template <class Traits>
class pool : public Traits
{
public:
    using traits = Traits;
    using typename traits::container_type;
    using traits::data;

protected:

    container_type pool_;

    template <class Rep, class Ratio>
    v1::block* block(const v1::page<Rep, Ratio>& page)
    {
        return reinterpret_cast<v1::block*>(data(pool_) + page_unit_type(page.pos()).count());
    }

    v1::bundle bundle(v1::bundle::page_type& page, unsigned handle)
    {
        return { block(page), &page, handle };
    }

public:
    ESTD_CPP_FORWARDING_CTOR_MEMBER(pool, pool_)

    using handle_type = int;

    template <class Traits2>
    typename Traits2::size_type alloc(handles<Traits2>&, unsigned logical_sz, unsigned block_sz);

    template <class Traits2>
    void dealloc(handles<Traits2>&, typename Traits2::size_type);
};

}}

inline namespace v1 {


}

}}
