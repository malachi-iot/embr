#pragma once

#include <cstring>

#include "../pool.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

// TODO: Consider also a brute relinker based purely on page.pos, though that's very extreme

template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::virtual_swap(bundle& lhs, bundle& rhs)
{
    constexpr handle_type null = block::null;
    // At its core we're doing a doubly-linked list item swap operation

    // Adjacency example - prev<-handle:block-pos:block->next

    // Scenario 1:
    // null<-0:0:A0->1, 0<-1:1:F0->2, 1<-2:2:A1->3, 2<-3:3:F1->null
    // A1 (lhs) wants to swap with F0 (rhs).  Note the counterintuitive order of lhs, rhs.
    // This is not unusual.  We then have:
    // null<-0:0:A0->1, 1<-1:2:A1->3, 0<-2:1:F0->2, 2<-3:3:F1->null
    // In addition to relinking neighbors, with adjacent blocks,
    // we have to relink the swapped blocks too:
    // 1.  Handle #1 prev must relink from 1 (itself) to 2
    // 2.  Handle #2 next must relink from 2 (itself) to 1
    // Creating
    // null<-0:0:A0->1, 2<-1:2:A1->3, 0<-2:1:F0->1, 2<-3:3:F1->null

    // Scenario 2:
    // null<-0:0:A0->1,  0<-1:1:F0->2 ,  1<-2:2:A1->3, 2<-3:3:F1->null
    // A0 (lhs) wants to swap with F0 (rhs):
    //   0<-0:1:F0->2 , null<-1:0:A0->1, 1<-2:2:A1->3, 2<-3:3:F1->null
    // Relinking required:
    // 1.  Handle# 0 prev must relink from 0 (itself) to 1
    // 2.  Handle# 1 next must relink from 1 (itself) to 0
    // Note how despite lhs movement from handle 0 to handle 1, that prev null link remains valid

    bundle lhs_prev(prev(lhs)), lhs_next(next(lhs));
    bundle rhs_prev(prev(rhs)), rhs_next(next(rhs));

    // Doesn't pass tests, but despite its mind bending nature should be close
    if(lhs.block->next() == rhs.handle)
    {
        lhs.next(lhs.handle);
        rhs.prev(rhs.handle);
    }
    else if(rhs.block->next() == lhs.handle)
    {
        rhs.next(rhs.handle);
        lhs.prev(lhs.handle);
    }

    if(lhs.has_prev())
    {
        lhs_prev.next(rhs.handle);
    }
    if(lhs.has_next())
    {
        lhs_next.prev(rhs.handle);
    }
    if(rhs.has_prev())
    {
        rhs_prev.next(lhs.handle);
    }
    if(rhs.has_next())
    {
        rhs_next.prev(lhs.handle);
    }

    // If my speculation is right, there are null-handle relinkings which still need to happen

    swap(*lhs.page, *rhs.page);
}


template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::virtual_move(bundle& from, bundle& to)
{
    assert(from.page->is_null() == false && to.page->is_null());

    bundle bn_prev(prev(from)), bn_next(next(from));

    if(from.has_prev()) bn_prev.next(to.handle);
    if(from.has_next()) bn_next.prev(to.handle);

    to.page->pos(from.pos());
    from.page->reset();
}

template <class Traits>
template <class HandleTraits>
v1::block* pool<Traits>::ops<HandleTraits>::move_block(page_type& page, pos_type new_pos)
{
    block* from = self_.block(page.pos());
    block* to = self_.block(new_pos);

    page.pos(new_pos);

    *to = *from;

    return to;
}

template <class Traits>
template <class HandleTraits>
v1::block* pool<Traits>::ops<HandleTraits>::resize(bundle bn, bundle bn_next, pos_type new_sz)
{
    const pos_type pos = bn.pos() + new_sz;

    return move_block(*bn_next.page, pos);
}

template <class Traits>
template <class HandleTraits>
v1::block* pool<Traits>::ops<HandleTraits>::resize(bundle bn, pos_type new_sz)
{
    assert(bn.has_next());
    bundle bn_next = next(bn);
    assert(bn_next.allocated() == false);
    const pos_type bn_sz = phys_size(bn);
    const pos_type bn_next_sz = phys_size(bn_next);
    assert(new_sz <= bn_sz + bn_next_sz);

    return resize(bn, bn_next, new_sz);
}


template <class Traits>
template <class HandleTraits>
validated_result pool<Traits>::ops<HandleTraits>::move(
    bundle from, bundle to,
    unsigned logical_sz,
    unsigned desired_logical_sz,
    bool is_overlapping)
{
    const bool is_trivial = from.mode() == block::Trivial;
    const pos_type to_block_phys_sz = phys_size(to);
    pos_type from_block_phys_sz = phys_size(from);

    const pos_type& free_block_phys_sz = to_block_phys_sz;
    const pos_type& alloced_block_phys_sz = from_block_phys_sz;
    /*
    const bool is_overlapping =
        (to.pos() < from.pos() && to.pos() + from_block_phys_sz > from.pos()) ||
        (to.pos() > from.pos() && from.pos() + from_block_phys_sz > to.pos()); */

    assert(from.page->is_null() == false && to.page->is_null() == false);

    // DEBT: Deducing logical_sz for non-trivial is interesting too, but not critical
    if(logical_sz == 0 && is_trivial)
    {
        logical_sz = logical_size(from);
    }

    if(is_overlapping)
    {
        assert(is_trivial);

        // Overlapping blocks mean that free block is smaller than allocated block

        if(from.pos() < to.pos())
        {
            // In this scenario we copy forwards.  Block 0 (starts as A)
            // header is preserved, while block 1 is overwritten.

            // A:0  ... F:1 -> F:0  .. A:1
            // from ... to     from .. to

            v1::block retained = *to.block;

            // New 'to' location moves backward enough to shrink preceding
            // 'from' block before it to match F size (keep F size consistent)
            pos_type new_to_loc = from.pos() + free_block_phys_sz;
            to.page->pos(new_to_loc);

            block* new_to_block = self_.block(new_to_loc);

            // moving A forward in memory = regular unfancy forward copy
            std::memcpy(new_to_block->data(), from.block->data(), logical_sz);

            *new_to_block = retained;
            new_to_block->reset(block::Trivial, true);
        }
        else
        {
            // In this scenario, we copy backawrds.  Block 0 (starts as F) header is
            // preserved, block 1 is overwritten

            // F:0 .. A:1  -> A:0 ... F:1
            // to  .. from    to  ... from

            v1::block retained = *from.block;

            // New 'from' location (1) moves forward to make room for expanding
            // 'to' block
            pos_type new_from_loc = to.pos() + alloced_block_phys_sz;
            from.page->pos(new_from_loc);

            block* new_from_block = self_.block(new_from_loc);

            // moving A backward in memory = fancy reverse copy
            std::memmove(to.block->data(), from.block->data(), logical_sz);

            *new_from_block = retained;
            to.block->reset(block::Trivial, true);

            from.block = new_from_block;
        }

        dealloc(from);

        // Not yet supported
        //assert(false);
    }
    else
    {
        assert(to_block_phys_sz >= from_block_phys_sz);

        // Adjacent, trivial blocks don't have to meet this requirement
        // TODO: Bring this check back for non adjacent OR non trivial blocks
        //assert(to_block_phys_sz >= from_block_phys_sz);

        to.block->move_from(from.block, logical_sz);
        to.allocated(true);

        dealloc(from);

        // New physical size is either directly the source block size or computed from
        // incoming desired logical size
        pos_type desired_phys_sz = desired_logical_sz == 0 ?
            from_block_phys_sz :
            pos_type((block::header_size(to.mode()) + desired_logical_sz) / aliasing);

        assert(desired_phys_sz <= to_block_phys_sz);

        //pos_type desired_phys_sz = from_block_phys_sz;

        // Resize 'to' to match old 'from'
        if(to_block_phys_sz != desired_phys_sz)
        {
            // Special treatment of to_next where we invite the 'one past end' block
            bundle to_next(next(to));
            bool has_next = to.has_next();
            bool to_next_allocated = has_next ? to_next.allocated() : true;
            pos_type to_next_pos = has_next ? to_next.pos() :
                pos_type(estd::size(self_.pool_) / aliasing);

            // We already fit neatly into 'to', so this is only to move following free block backward
            if(to_next_allocated == false)
                resize(to, to_next, desired_phys_sz);
            else
            {
                // If following block is allocated, instead see if we can do a split to create a mini
                // free block

                // DEBT: Consolidate with the logic in alloc
                constexpr pos_type split_threshold{3};
                // We do not use existing calculated size because 'dealloc' may have changed free
                // block landscape
                // DEBT: Although probably not, since if to_next is allocated so far there are no use cases
                // which would move to_next around before we get here
                const pos_type phys_sz = to_next_pos - to.pos();
                const pos_type delta = phys_sz - desired_phys_sz;

                // If wasted alloc space count is large enough to split, do so
                if(delta >= split_threshold)
                {
                    const pos_type split_pos = to_next_pos - delta;
                    //const pos_type new_pos2 = to.pos() + from_block_phys_sz;
                    //assert(new_pos1 == new_pos2);
                    // DEBT: Much like alloc, this is a bit harsh
                    assert(split_at(to, split_pos));
                }
                else
                {
                    // DEBT: If not, we have a secret fragmentation hiding in extra-allocated space here.
                    // Some extra movements may be appropriate here, revisit
                }
            }
        }
    }

    // Treat move as the dealloc it is, and do a merge evaluation
    //merge(from, next(from));
    //merge_if_free(prev(from), from);

    return {};
}

}}

}}
