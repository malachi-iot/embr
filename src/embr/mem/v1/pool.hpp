#pragma once

#include <estd/internal/rtto.h>
#include <estd/new.h>
#include <estd/numeric.h>

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
        handle_type i = &p - &handles_[0];

        bundle bn = get_bundle(p, i);

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
    return { self_.block(page.pos()), &page, b->prev() };
}

template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::next(const v1::block* b) const -> bundle
{
    page_type& page = handles_[b->next()];
    return { self_.block(page.pos()), &page, b->next() };
}


template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::phys_size(const bundle& bn) const -> pos_type
{
    pos_type next_pos = bn.block->next() == handles_type::null ?
        pos_type(std::size(self_.pool_) / aliasing) :
        next(bn).pos();

    //return next(bn).pos() - bn.pos();
    return next_pos - bn.pos();
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
auto pool<Traits>::ops<HandleTraits>::create_free_block(
    pos_type pos,
    handle_type prev, handle_type next) -> block*
{
    block* storage = self_.block(pos);

    return new (storage) block(block::Trivial, false, prev, next);
}

template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::split(bundle b, pos_type at) -> handle_type
{
    // Brand new handle needed for this
    return handles_.alloc([&](handle_type h, page_type& page)
    {
        page.pos(at);

        create_free_block(at, b.handle, b.block->next());

        b.next(h);
    });
}

template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::reset()
{
    using unit_type = typename page_type::unit_type;
    handles_.reset();
    handles_[0].pos(unit_type(0));
    //v1::bundle bn = self_.bundle(handles_[0], 0);

    create_free_block(pos_type(0), block::null, block::null);

    //new (bn.block) v1::block(v1::block::Trivial, false);
}


template <class Traits>
template <class HandleTraits>
template <block::modes mode>
auto pool<Traits>::ops<HandleTraits>::alloc(pos_type phys_sz) -> bundle
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
auto pool<Traits>::ops<HandleTraits>::construct(Args&&...args) -> bundle
{
    // Rtto and Immobile use proxy
    constexpr bool rtto_proxied = mode == block::RttoProxy || mode == block::Immobile;
    // DEBT: Effective but error prone accounting for various block sizing.  Probably
    // ought to move this plumbing into 'emplace'
    constexpr unsigned block_sz = block::header_size<mode>();

    bundle bn = alloc<mode>(do_alias(sizeof(T) + block_sz));

    if(bn.is_null())    return {};

    // DEBT: May need this to be if constexpr (or equivalent) - keep an eye on this
    if(rtto_proxied)
        bn.block->emplace_rtto_proxied<T>(std::forward<Args>(args)...);
    else
        bn.block->emplace<T>(std::forward<Args>(args)...);

    return bn;
}


template <class Traits>
template <class HandleTraits>
bool pool<Traits>::ops<HandleTraits>::merge(bundle current, bundle next)
{
    if(next.block->allocated()) return false;

    // Remove this handle from the pool completely
    handles_.dealloc(next.handle);

    current.next(next.block->next());

    return true;
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

    return ops<Traits2>{*this, h}.template construct<mode, T>(std::forward<Args>(args)...).handle;
}

template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::dealloc(bundle bn)
{
    // DEBT: Consolidate with other free operations
    bn.block->destroy();

    bn.block->allocated(false);

    if(bn.has_prev())   merge(prev(bn), bn);
    if(bn.has_next())   merge(bn, next(bn));
}


template <class Traits>
template <class HandleTraits>
void* pool<Traits>::ops<HandleTraits>::lock(handle_type h)
{
    bundle bn(get_bundle(h));

    bn.lock_up();

    return bn.data();
}

template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::unlock(handle_type h)
{
    bundle bn(get_bundle(h));

    bn.lock_down();
}

template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::ref_up(handle_type h)
{
    bundle bn(get_bundle(h));

    bn.ref_up();
}


template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::ref_down(handle_type h)
{
    bundle bn(get_bundle(h));

    if(bn.ref_down() == 0)
    {
        dealloc(bn);
    }
}


template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::available() const -> unsigned
{
    // If we permitted ourselves c++20 we could use ranges here.  Oh well !

    unsigned count(0);

    for(page_type& page : handles_)
    {
        bundle bn = get_bundle(page);

        if(bn.block->allocated() == false)  count += logical_size(bn);
    }

    return count;
}


template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::alloced() const -> unsigned
{
    // If we permitted ourselves c++20 we could use ranges here.  Oh well !

    return estd::accumulate(handles_.begin(), handles_.end(), 0, [&](unsigned count, page_type& p)
    {
        bundle bn = get_bundle(p);

        return count + (bn.block->allocated() ? logical_size(bn) : 0);
    });
}


}}

}}
