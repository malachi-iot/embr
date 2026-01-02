#pragma once

#include <estd/internal/rtto.h>
#include <estd/new.h>
#include <estd/numeric.h>

#if FEATURE_STD_OSTREAM
#include <iostream>
#endif

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
template <class Traits2, class Block, class Page>
void pool<Traits>::ops<HandleTraits>::prev(const v1::block* b, bundle_base<Traits2, Block, Page>* out) const
{
    page_type& page = handles_[b->prev()];
    new (out) bundle_base<traits, Block, Page>{ self_.block(page.pos()), &page, b->prev() };
}


template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::prev(const v1::block* b) const -> bundle
{
    page_type& page = handles_[b->prev()];
    return { self_.block(page.pos()), &page, b->prev() };
}

template <class Traits>
template <class HandleTraits>
template <class Traits2, class Block, class Page>
void pool<Traits>::ops<HandleTraits>::next(const v1::block* b, bundle_base<Traits2, Block, Page>* out) const
{
    page_type& page = handles_[b->next()];
    new (out) bundle_base<traits, Block, Page>{ self_.block(page.pos()), &page, b->next() };
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
template <class Traits2, class Block, class Page>
auto pool<Traits>::ops<HandleTraits>::phys_size(const bundle_base<Traits2, Block, Page>& bn) const -> pos_type
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

    create_free_block(pos_type(0), block::null, block::null);

}


template <class Traits>
template <class HandleTraits>
auto pool<Traits>::ops<HandleTraits>::alloc(pos_type phys_sz, block::modes mode) -> bundle
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
            pos_type at = bn.page->pos() + phys_sz;

            // DEBT: An assert is a little too harsh here, but helpful enough to keep for the short term
            // really we need an error code
            assert(split(bn, at) != handles_type::traits::null);
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

    bundle bn = alloc(do_alias(sizeof(T) + block_sz), mode);

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
    if(next.allocated()) return false;

    // Link next1->next2->prev to us, since next1 is going away
    if(next.has_next())
    {
        this->next(next).prev(current.handle);
    }

    // Remove the 'next' handle from the pool completely
    handles_.dealloc(next.handle);

    current.next(next.block->next());

    return true;
}

template <class Traits>
template <class HandleTraits>
bool pool<Traits>::ops<HandleTraits>::merge_free(bundle current, bundle next)
{
    if(current.allocated()) return false;

    return merge(current, next);
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
    to.allocated(true);
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

    bn.allocated(false);

    if(bn.has_prev())
    {
        bundle bn_prev = prev(bn);
        // this potentially nulls out bn.handle and invalidates its block
        if(merge_free(bn_prev, bn))
            // if so, we've merged with previous, so make him the one
            // we evaluate the following merge next against
            bn = bn_prev;
    }

    if(bn.has_next())
        merge_free(bn, next(bn));
}


template <class Traits>
template <class HandleTraits>
void* pool<Traits>::ops<HandleTraits>::lock(bundle bn)
{
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


template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::defrag(const fragmentation::candidate& c)
{
    // DEBT: A bit sloppy converting bundles like this, but gets the job done
    move(get_bundle(c.bundle.handle), get_bundle(c.move_to.handle), 0);
}

template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::assess(fragmentation* frag) const
{
    const v1::block* b = self_.block(pos_type(0));

    if(b->next() == v1::block::null)    return;

    const_bundle bn_prev{}, bn_next, cur;
    next(b, &bn_next);
    prev(bn_next.block, &cur);
    int last_sc = 0;

    fragmentation::candidate& top = frag->candidates[0];
    top = {};
    frag->candidates[1] = {};

    static constexpr uint16_t booster = 4;

    auto score = [&](const_bundle prev, const_bundle cur, const_bundle next)
    {
        pos_type v(0);

        // favor a triple with a small middle, since that's easier to move
        // DEBT: See https://github.com/malachi-iot/estdlib/issues/155 - specifically
        // I'm considering a flag permit to precision loss, though I kind of like that
        // it caught this and errored as it should have.  "booster" HAS to be uint16_t, but really
        // multiplication implies a precision loss anyway
        v += phys_size(prev) * booster;
        v += phys_size(cur);
        v += phys_size(next) * booster;

        return v.count();
    };

    pos_type bn_cur_sz = phys_size(cur);

    while(cur.handle != block::null)
    {
        if(cur.is_null() == false && bn_prev.is_null() == false)
        {
            pos_type bn_prev_sz = bn_cur_sz;
            pos_type bn_next_sz = phys_size(bn_next);
            bn_cur_sz = phys_size(cur);

            // F A F
            if(!bn_prev.allocated() && cur.allocated() && !bn_next.allocated())
            {
                //int sc = score(bn_prev, cur, bn_next);
                pos_type v(0);

                // favor trivial
                // favor a triple with a small middle, since that's easier to move

                if(cur.is_trivial())
                {
                    v += bn_prev_sz * booster;
                    v += bn_cur_sz;
                    v += bn_next_sz * booster;
                }
                else
                {
                    if(bn_cur_sz <= bn_prev_sz) v += bn_prev_sz;
                    if(bn_cur_sz <= bn_next_sz) v += bn_next_sz;
                }

                int sc = v.count();

                if(sc > last_sc)
                {
                    estd::swap(frag->candidates[0], frag->candidates[1]);
                    top = { sc, cur, bn_prev };
                    last_sc = sc;
                }
            }
            // A F A
            else if(bn_prev.allocated() && !cur.allocated() && bn_next.allocated())
            {
                // favor trivial
                // favor one of A <= F, smaller is better, and demand it if non-trivial
                // favor above F A F pattern over this one

                pos_type v(0);
                const_bundle* which = &bn_prev;     // Presume bn_prev is the more interesting candidate

                // DEBT: Assess bn_next too inside this branch
                if(bn_prev.is_trivial())
                {
                    v += bn_prev_sz;
                    v += bn_cur_sz * booster;
                    //v += bn_next_sz;

                    // DEBT: For trivial, it matters a little less which allocated block we move.  Still though,
                    // we'd like to choose the smaller of the two and not only presume bn_prev as above
                }
                else
                {
                    using signed_type = estd::units::v1::detail::unit<page_unit_traits<int, typename pos_type::period>>;
                    static constexpr signed_type zero(0);
                    signed_type prev_delta = bn_cur_sz - bn_prev_sz;
                    signed_type next_delta = bn_cur_sz - bn_next_sz;


                    if(prev_delta >= zero)
                    {
                        // DEBT: I'm actually surprised estd::units permits this addition of an int to a uint16_t
                        v = prev_delta;
                    }

                    if(bn_next.is_trivial())
                    {
                        // DEBT: We do favor trivial, but shouldn't hard-select it
                        which = &bn_next;
                        //v += bn_prev_sz;
                        v += bn_cur_sz * booster;
                        v += bn_next_sz;
                    }
                    else if(next_delta >= zero)
                    {
                        // If 'prev' isn't viable OR next is smaller than prev, select 'next'
                        if(prev_delta < zero || next_delta < prev_delta) which = &bn_next;

                        v = next_delta;
                    }
                }

                int sc = v.count();

                if(sc > last_sc)
                {
                    estd::swap(frag->candidates[0], frag->candidates[1]);
                    top = { sc, *which, cur };
                    last_sc = sc;
                }
            }
        }

        bn_prev = cur;
        cur = bn_next;
        next(cur.block, &bn_next);
    }
}

template <class Traits>
template <class HandleTraits>
invariant_result pool<Traits>::ops<HandleTraits>::invariant() const
{
    using result = invariant_result::unexpected_type;
    using violation = invariant_violation;
    //using iterator = typename handles_type::const_iterator;
    const page_type* first{};
    constexpr pos_type zero_pos = pos_type(0);
    // TODO: Inspires a thought of convertible-to which embr/estd units explored before.  pos_type
    // really is directly convertible to bytes - although in this case we could probably cheat and
    // use bytes_tag type right from the get go
    using bytes_type = estd::units::v1::detail::unit<page_unit_traits<unsigned, estd::ratio<1>>>;
    const bytes_type size(std::size(self_.pool_));
    constexpr handle_type null = traits::null;

    // Scan for page representing position 0
    for(const page_type& page : handles_)
    {
        if(page.pos() == zero_pos)
        {
            first = &page;
            break;
        }

        //const_bundle bn = get_bundle(page);
        //if(bn.p)
    }

    // Minimum one handle MUST be allocated at all times (big free block)
    if(first == nullptr)
#if FEATURE_EMBR_MEM_INVARIANT_BOOL
        return false;
#else
        return result({"no handles", ""});
#endif

    // Now, walk forward and check that 'next' is sane
    const_bundle bn = get_bundle(*first);
    const_bundle bn_last{};
    pos_type size_tally{0};

    for(; bn.handle != null; next(bn.block, &bn), bn)
    {
        // As we walk forward, if physical position moves backward, that's an error
        if(!bn_last.is_null())
            if(bn.pos() < bn_last.pos())
#if FEATURE_EMBR_MEM_INVARIANT_BOOL
                return false;
#else
                return result({"position check failed", "during next check"});
#endif
        if(bn.page->is_null())
            return result({"null page encountered", "during next check"});

        size_tally += phys_size(bn);

        bn_last = bn;
    }

    if(size_tally != size)
#if FEATURE_EMBR_MEM_INVARIANT_BOOL
        return false;
#else
        return result({"size tally failed", "during next check"});
#endif

    size_tally = zero_pos;

    // Walk backward and check that 'prev' is sane
    for(bn = bn_last; bn.handle != null; prev(bn.block, &bn), bn)
    {
        // As we walk backward, if physical position moves forward, that's an error
        if(bn.pos() > bn_last.pos())
#if FEATURE_EMBR_MEM_INVARIANT_BOOL
            return false;
#else
            return result({"position check failed", "during prev check"});
#endif

        size_tally += phys_size(bn);

        bn_last = bn;
    }

    if(size_tally != size)
#if FEATURE_EMBR_MEM_INVARIANT_BOOL
        return false;
#else
        return result({"size tally failed", "during prev check"});
#endif

#if FEATURE_EMBR_MEM_INVARIANT_BOOL
    return true;
#else
    return {};
#endif
}


#if FEATURE_STD_OSTREAM
template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::dump(std::ostream& out) const
{
    using bytes_type = estd::units::v1::detail::unit<page_unit_traits<unsigned, estd::ratio<1>>>;
    constexpr handle_type null = traits::null;
    constexpr pos_type zero_pos = pos_type(0);
    const page_type* first{};
    // Scan for page representing position 0
    for(const page_type& page : handles_)
    {
        if(page.pos() == zero_pos)
        {
            first = &page;
            break;
        }
    }

    if(first == nullptr)
    {
        out << "Couldn't find first page";
        return;
    }

    for(const_bundle bn = get_bundle(*first); bn.handle != null; next(bn.block, &bn), bn)
    {
        out << "Bundle: handle=" << (int)bn.handle;
        if(bn.page->is_null())
        {
            out << " null page - abort\n";
            break;
        }
        bytes_type sz = phys_size(bn);
        out << ", " << (bn.allocated() ? "A" : "F");
        out << ", prev=" << (int)bn.block->prev();
        out << ", next=" << (int)bn.block->next();
        out << ", sz=" << sz.count() << "b";
        out << ", pos=" << bytes_type(bn.pos()).count();
        //out << ", block=" << bn.block;
        out << '\n';
    }

    out.flush();
}
#endif

}}

}}
