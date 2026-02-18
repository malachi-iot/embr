#pragma once

#include <estd/internal/rtto.h>
#include <estd/mutex.h>
#include <estd/new.h>
#include <estd/numeric.h>

#if FEATURE_STD_OSTREAM
#include <iostream>
#endif

#include "block.hpp"
#include "pool.h"
#include "pool/construct.hpp"
#include "pool/copy.hpp"
#include "pool/defrag.hpp"
#include "pool/move.hpp"
#include "pool/invariant.hpp"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

template <class Traits>
auto pool_ops<Traits>::first() const -> const_bundle
{
    const page_type* page = handles_.first_zero();

    return page ? get_bundle(*page) : const_bundle{};
}

template <class Traits>
auto pool_ops<Traits>::first_free(pos_type phys_sz, pos_type* found_size) const -> const_bundle
{
    for(const page_type& p : handles_)
    {
        const_bundle bn = get_bundle(p);

        if(bn.allocated() == false)
        {
            pos_type candidate_sz = phys_size(bn);

            if(candidate_sz >= phys_sz)
            {
                *found_size = candidate_sz;
                return bn;
            }
        }
    }

    return {};
}

template <class Traits>
template <class Block>
void pool_ops<Traits>::next(Block* b, bundle_base<handles_traits, Block>* out) const
{
    // DEBT: No auto please
    auto& page = handles_[b->next()];
    new (out) bundle_base<handles_traits, Block>{ storage().block(page.pos()), &page, b->next() };
}


template <class Traits>
auto pool_ops<Traits>::phys_size(const const_bundle& bn) const -> pos_type
{
    pos_type next_pos = bn.block->next() == handles_type::null ?
        pos_type(estd::size(storage().pool_) / aliasing) :
        next(bn).pos();

    //return next(bn).pos() - bn.pos();
    return next_pos - bn.pos();
}

template <class Traits>
estd::units::bytes<unsigned> pool_ops<Traits>::logical_size(block::modes mode, pos_type phys_sz)
{
    // FIX: Needs more work
    const unsigned tbd = block::header_size(mode).count();
    return estd::units::bytes<unsigned>(phys_sz.count() * aliasing - tbd);
}

template <class Traits>
estd::units::bytes<unsigned> pool_ops<Traits>::logical_size(const const_bundle& bn) const
{
    return logical_size(bn.block->mode(), phys_size(bn));
}

template <class Traits>
ESTD_CPP_CONSTEXPR(14) auto pool_ops<Traits>::create_free_block(
    pos_type pos,
    handle_type prev, handle_type next) -> block*
{
    block* storage = storage_.block(pos);

    return new (storage) block(block::Trivial, false, prev, next);
}

template <class Traits>
auto pool_ops<Traits>::split_at(const bundle& b, pos_type at) -> handle_type
{
    // Brand new handle needed for this
    return handles_.alloc([&](handle_type h, page_type& page)
    {
        // At new handle, assign split point for new block location
        page.pos(at);

        create_free_block(at, b.handle, b.block->next());

        // if presented bundle b is followed by a block, splice in 'h'
        // before b.next (traditional linked list insert)
        if(b.has_next())    next(b).prev(h);

        b.next(h);
    });
}

template <class Traits>
ESTD_CPP_CONSTEXPR(14) void pool_ops<Traits>::reset()
{
    // Note we use pos_type(0) and not 'null' because first free block is NOT null -
    // it's an allocated handle to a free block
    handles_.reset();
    handles_[0].pos(pos_type(0));

    create_free_block(pos_type(0), block::null, block::null);

}


template <class Traits>
void pool_ops<Traits>::alloc(const bundle& bn, pos_type found_sz, pos_type phys_sz, block::modes mode)
{
    // If we're 3 blocks larger, go ahead and split
    // We start with min block size.  On 64-bit systems that is:
    // 8 for header, 8 for free data portion.  We then fudge it
    // and say that either:
    // 1. Room for RttoProxy mode is interesting OR
    // 2. Room for larger than the tiniest free block is interesting
    // So we add +1
    // NOTE: May need tuning
    constexpr pos_type min_block_and_data_phys_sz(2);
    constexpr pos_type split_threshold{min_block_and_data_phys_sz + pos_type(1)};

    if(found_sz - phys_sz >= split_threshold)
    {
        pos_type at = bn.pos() + phys_sz;

        // DEBT: An assert is a little too harsh here, but helpful enough to keep for the short term
        // really we need an error code
        assert(split_at(bn, at) != handles_type::traits::null);
    }

    bn.block->reset(mode, true);
}


template <class Traits>
auto pool_ops<Traits>::alloc(pos_type phys_sz, block::modes mode) -> bundle
{
    pos_type found_size(0);

    bundle bn = first_free(phys_sz, &found_size).unconst();

    if(bn.is_null() == false)   alloc(bn, found_size, phys_sz, mode);

    //assert(phys_size(bn) >= pos_type(2));

    return bn;
}


template <class Traits>
bool pool_ops<Traits>::merge(bundle current, bundle next)
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
bool pool_ops<Traits>::merge_if_free(bundle current, bundle next)
{
    if(current.allocated()) return false;

    return merge(current, next);
}

template <class Traits>
void pool_ops<Traits>::dealloc(bundle bn)
{
    // DEBT: Consolidate with other free operations
    bn.block->destroy();

    bn.block->reset(block::Trivial, false);

    if(bn.has_prev())
    {
        bundle bn_prev = prev(bn);
        // this potentially nulls out bn.handle and invalidates its block
        if(merge_if_free(bn_prev, bn))
            // if so, we've merged with previous, so make him the one
            // we evaluate the following merge next against
            bn = bn_prev;
    }

    if(bn.has_next())
        merge_if_free(bn, next(bn));

    //assert(phys_size(bn) >= pos_type(2));
    //assert(bn.invariant());
}


template <class Traits>
void* pool_ops<Traits>::lock(bundle bn)
{
    bn.lock_up();

    return bn.data();
}


template <class Traits>
template <class Mutex>
void* pool_ops<Traits>::lock(handle_type h, Mutex mutex)
{
#if __has_builtin(__atomic_compare_exchange_n)
#endif

    estd::lock_guard<Mutex> lg(mutex);

    return lock(get_bundle(h));
}

template <class Traits>
template <class Mutex>
void pool_ops<Traits>::unlock(handle_type h, Mutex mutex)
{
    estd::lock_guard<Mutex> lg(mutex);

    bundle bn(get_bundle(h));

    bn.lock_down();
}

template <class Traits>
void pool_ops<Traits>::ref_up(handle_type h)
{
    bundle bn(get_bundle(h));

    bn.ref_up();
}


template <class Traits>
void pool_ops<Traits>::ref_down(handle_type h)
{
    bundle bn(get_bundle(h));

    if(bn.ref_down() == 0)
    {
        dealloc(bn);
    }
}


template <class Traits>
auto pool_ops<Traits>::available() const -> bytes
{
    // If we permitted ourselves c++20 we could use ranges here.  Oh well !

    bytes count(0);

    for(const page_type& page : handles_)
    {
        const_bundle bn = get_bundle(page);

        if(bn.allocated() == false)  count += logical_size(bn);
    }

    return count;
}


template <class Traits>
auto pool_ops<Traits>::alloced() const -> bytes
{
    // If we permitted ourselves c++20 we could use ranges here.  Oh well !

    return bytes(estd::accumulate(handles_.begin(), handles_.end(), 0, [&](unsigned count, const page_type& p)
    {
        const_bundle bn = get_bundle(p);

        return count + (bn.allocated() ? logical_size(bn).count() : 0);
    }));
}


template <class Traits>
bool pool_ops<Traits>::realloc(bundle bn, pos_type phys_sz)
{
    // If sz <= phys_sz then just return
    // If sz > phys sz then:
    // 1.  If following free block exists and can accomodate us, shrink it down and grow into it
    // 2.  Otherwise look at alternate locations to allocate from (probably assess)
    // 2.a.  A chunk big enough is just hanging around, ideally similarly sized
    // 2.b.  We issue a single defrag to see if we can get more space, then try again

    pos_type current_sz = phys_size(bn);

    if(phys_sz <= current_sz)
        return true;

    if(bn.has_next())
    {
        // Assess for condition #1

        bundle bn_next(next(bn));

        if(bn_next.allocated() == false)
        {
            // Reaching here means we have a following free block that possibly we can expand into

            pos_type next_sz = phys_size(bn_next);

            // DEBT: We do this kind of op elsewhere (forget where) - consolidate
            // DEBT: This also feels like we overlap with 'resize' - consolidate

            const pos_type max_sz(current_sz + next_sz);

            // If requested phys_sz does not exceed actual current size + available free block...
            if(phys_sz <= max_sz)
            {
                // ... then we can comfortably grow the block

                // DEBT: Document why '3' is good (or not)
                constexpr pos_type split_thresh(3);

                if(max_sz - phys_sz >= split_thresh)
                {
                    // retain existing free block, merely move its starting point to
                    // the end of the newly grown block
                    move_block(*bn_next.page, bn.pos() + phys_sz);
                }
                else
                    handles_.dealloc(bn_next.handle);

                return true;
            }

            // Reaching here means requested size won't fit, so cascade down to assess a move operation instead
        }
    }

    fragmentation frag;

    auto move_to_candidate = [&]
    {
        assess(&frag);

        bundle dest(get_bundle(frag.largest_free_handle));

        pos_type free_sz = phys_size(dest);

        if(free_sz < current_sz)    return false;

        unsigned logical_sz = logical_size(bn.mode(), phys_sz).count();

        move(bn, dest, 0, logical_sz, false);

        //block bn_saved = *bn.block;
        //block dest_saved = *dest.block;

        // Example - prev<-handle:block-pos:block->next
        // null<-0:0:A0->1, 0<-1:1:F->2, 1<-2:2:A1->3, 2<-3:3:F->null
        // A0 wants to realloc to 3:3:F.  We become
        // null<-0:0:F->1,  0<-1:1:F->2, 1<-2:2:A1->3, 2<-3:3:A0->4,   3<-4:4:F->null
        // Since previous 3:3:F splits and creates 4:4:F
        // Now we swap page table position 3 and 0 so that A0 handle remains constant
        // 2<-0:3:A0->4,    0<-1:1:F->2, 1<-2:2:A1->3, null<-3:0:F->1, 3<-4:4:F->null
        // 0:A0 is still physically at old position 3
        // Critically, block-pos must remain contiguous.  This means:
        // 1. handle #1 should now prev link to handle 3, not 0
        // 2. handle #2 should now next link to handle 0, not 3
        // 3. handle #4 should now prev link to handle 0, not 3
        // Creating:
        // 2<-0:3:A0->4,    3<-1:1:F->2, 1<-2:2:A1->0, null<-3:0:F->1, 0<-4:4:F->null
        // Although this is a little mind bending, remember the golden rule where prev/next MUST represent contiguous
        // block-positions

        // Swap page table positions and relink as described above
        virtual_swap(bn, dest);

        return true;
    };

    // If move succeeds, return and indicate so
    if(move_to_candidate()) return true;

    // If not, evaluate whether we can defrag
    if(frag.candidates[0].score == 0) return false;

    // Attempt another move after a single defrag
    defrag(frag.candidates[0]);
    return move_to_candidate();
}


}}

}}
