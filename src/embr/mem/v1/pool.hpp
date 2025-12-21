#pragma once

#include <estd/internal/rtto.h>
#include <estd/new.h>

// DEBT: https://github.com/malachi-iot/estdlib/issues/155
#include <estd/internal/units/operators.hpp>

#include "block.hpp"
#include "pool.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::first_free(pos_type phys_sz, pos_type* found_size) const -> bundle
{
    for(page_type& p : handles_)
    {
        int i = &p - &handles_[0];

        bundle bn = self_.bundle(p, i);

        if(bn.block->allocated() == false)
        {
            pos_type candidate_sz = phys_size(bn);

            if(candidate_sz >= phys_sz)
            {
                *found_size = candidate_sz;
                return bn;
            }
        }

        // Nifty, but needs this clumsy end check since 'end' doesn't work normally here
        if(&p == handles_.end().base() - 1)
        {
            return {};
        }
    }

    return {};
}

// FIX: These next/prev guys need bounds checking

template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::prev(const v1::block* b) const -> bundle
{
    page_type& page = handles_[b->prev()];
    return { self_.block(page), &page, b->prev() };
}

template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::next(const v1::block* b) const -> bundle
{
    page_type& page = handles_[b->next()];
    return { self_.block(page), &page, b->next() };
}


template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::phys_size(const bundle& bn) const -> pos_type
{
    return next(bn).pos() - bn.pos();
}

template <class Traits>
template <class HandleTraits>
unsigned pool<Traits>::ops<HandleTraits>::logical_size(const bundle& bn) const
{
    // FIX: Needs more work
    const unsigned tbd = bn.block->header_size<block::Trivial>();
    return phys_size(bn).count() * aliasing - tbd;
}

template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::split(bundle b, pos_type at) -> handle_type
{
    // Brand new handle needed for this
    return handles_.alloc([&](handle_type h, page_type& page)
    {
        block* storage = self_.block(page);

        new (storage) block(block::Trivial, false, b.handle, b.block->next());

        b.block->next(h);
    });
}

template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::reset()
{
    using unit_type = typename page_type::unit_type;
    handles_.reset();
    handles_[0].pos(unit_type(0));
    v1::bundle bn = self_.bundle(handles_[0], 0);

    new (bn.block) v1::block(v1::block::Trivial, false);
}

template <class Traits>
template <class HandleTraits>
template <v1::block::modes mode>
auto pool<Traits>::ops<HandleTraits>::alloc_old(unsigned phys_sz, v1::bundle* bn) -> handle_type
{
    return handles_.alloc(
        [&](int h, page_type& page)
        {
            if(page.is_null()) return false;
            *bn = self_.bundle(page, h);     // semi-side-effect
            if(bn->block->allocated())  return false;
            unsigned candidate_phys_sz = phys_size(*bn).count() * aliasing;
            return candidate_phys_sz >= phys_sz;
        },
        [&](int, page_type&)
        {
            new (bn->block) v1::block(mode, true);
        });
}

template <class Traits>
template <class HandleTraits>
template <block::modes mode>
auto pool<Traits>::ops<HandleTraits>::alloc_new(pos_type phys_sz) -> bundle
{
    pos_type found_size(0);
    bundle bn = first_free(phys_sz, &found_size);

    if(bn.is_null() == false)
    {
        // If we're 3 blocks larger, go ahead and split
        // NOTE: Will need tuning
        constexpr pos_type split_threshold{3};

        if(found_size - phys_sz >= split_threshold)
        {
            assert(split(bn, phys_sz) != handles_type::traits::null);
        }

        bn.block->reset(mode, true);
    }

    return bn;
}

template <class Traits>
template <class HandleTraits>
template <block::modes mode, class T, class ...Args>
auto pool<Traits>::ops<HandleTraits>::construct(bundle* bn, Args&&...args) -> handle_type
{
    // Rtto and Immobile use proxy
    constexpr bool rtto_proxied = mode == block::RttoProxy || mode == block::Immobile;
    // DEBT: Effective but error prone accounting for various block sizing.  Probably
    // ought to move this plumbing into 'emplace'
    constexpr unsigned block_sz = block::header_size<mode>();
    constexpr unsigned phys_sz = sizeof(T) + block_sz;
    // DEBT: Crude way to get full phys_sz
    constexpr pos_type phys_sz2((phys_sz + aliasing - 1) / aliasing);

    *bn = alloc_new<mode>(phys_sz2);

    if(bn->is_null())    return handles_type::traits::null;

    // DEBT: May need this to be if constexpr (or equivalent) - keep an eye on this
    if(rtto_proxied)
        bn->block->emplace_rtto_proxied<T>(std::forward<Args>(args)...);
    else
        bn->block->emplace<T>(std::forward<Args>(args)...);

    return bn->handle;

    /*
    return handles_.alloc(
        [&](int h, page_type& page)
        {
            if(page.is_null()) return false;
            *bn = self_.bundle(page, h);     // semi-side-effect
            if(bn->block->allocated())  return false;
            unsigned candidate_phys_sz = phys_size(*bn).count() * aliasing;
            return candidate_phys_sz >= phys_sz;
        },
        [&](int, page_type&)
        {
            new (bn->block) v1::block(
                estd::in_place_index_t<mode>{},
                estd::in_place_type_t<T>{},
                std::forward<Args>(args)...);
        }); */
}


template <class Traits>
template <class HandleTraits>
bool pool<Traits>::ops<HandleTraits>::merge(bundle current, bundle next)
{
    if(next.block->allocated()) return false;

    // Remove this handle from the pool completely
    handles_.dealloc(next.handle);

    current.block->next(next.block->next());
}

template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::move(bundle from, bundle to, unsigned logical_sz)
{
    // DEBT: Deducing logical_sz for non-trivial is interesting too, but not critical
    if(logical_sz == 0 && from.block->mode() == block::Trivial)
    {
        logical_sz = logical_size(from);
    }

    to.block->move_from(from.block, logical_sz);
    from.block->reset(block::Trivial, false);

    // Treat move as the dealloc it is, and do a merge evaluation
    merge(from, next(from));
    bundle p = prev(from);
    if(p.block->allocated() == false)   merge(p, from);
}



template <class Traits>
template <class T, class Traits2, class ...Args>
typename Traits2::size_type pool<Traits>::construct(handles<Traits2>& h, Args&&...args)
{
    using is_trivial = estd::is_trivially_constructible<T>;
    using is_movable = estd::is_move_constructible<T>;
    using is_rtto_base = estd::is_base_of<estd::internal::rtto_base::base, T>;
    constexpr v1::block::modes mode =
        is_trivial::value ? v1::block::Trivial :
        !is_movable::value ? v1::block::Immobile :
        is_rtto_base::value ? v1::block::RttoBase : v1::block::RttoProxy;

    v1::bundle bn;
    handle_type handle = ops<Traits2>{*this, h}.template construct<mode, T>(&bn, std::forward<Args>(args)...);
    return handle;
}


}}

}}
