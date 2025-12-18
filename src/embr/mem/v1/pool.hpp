#pragma once

#include "block.h"
#include "pool.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Traits>
template <class HandleTraits>
v1::bundle pool<Traits>::ops<HandleTraits>::next(const v1::block* b)
{
    page_type& page = handles_[b->next()];
    return { self_.block(page), &page, b->next() };
}


template <class Traits>
template <class HandleTraits>
unsigned pool<Traits>::ops<HandleTraits>::phys_size(const v1::bundle& b)
{
    v1::bundle n = next(b.block);

    // https://github.com/malachi-iot/estdlib/issues/155
    //auto ret = n.page->pos() - b.page->pos();
    unsigned ret = n.page->pos().count() - b.page->pos().count();
    return ret * aliasing;
}

template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::alloc(unsigned phys_sz) -> handle_type
{
    return handles_.alloc(
        [&](int h, page_type& page)
        {
            v1::bundle bn = self_.bundle(page, h);
            unsigned candidate_phys_sz = phys_size(bn);
            return candidate_phys_sz >= phys_sz;
        },
        [&](auto& v)
        {
        });
}


}}

}}
