#include <estd/internal/rtto.h>
#include <estd/new.h>
#include <estd/numeric.h>

#include "../block.hpp"
#include "../pool.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {


// TODO: Consider returning invariant result
template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::move(bundle from, bundle to, unsigned logical_sz,
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

        // Almost works, but makes assess unit test a little upset.  Unclear whether we or the test iself
        // is at fault
        //pos_type desired_phys_sz = logical_sz == 0 ? from_block_phys_sz : pos_type(logical_sz / aliasing);
        pos_type desired_phys_sz = from_block_phys_sz;

        // Resize 'to' to match old 'from'
        if(to.has_next() && to_block_phys_sz != desired_phys_sz)
        {
            bundle to_next(next(to));

            // We already fit neatly into 'to', so this is only to move following free block backward
            if(to_next.allocated() == false)
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
                const pos_type phys_sz = to_next.pos() - to.pos();
                const pos_type delta = phys_sz - desired_phys_sz;

                // If wasted alloc space count is large enough to split, do so
                if(delta >= split_threshold)
                {
                    const pos_type new_pos1 = to_next.pos() - delta;
                    //const pos_type new_pos2 = to.pos() + from_block_phys_sz;
                    //assert(new_pos1 == new_pos2);
                    // DEBT: Much like alloc, this is a bit harsh
                    assert(split_at(to, new_pos1));
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
}



template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::defrag(const fragmentation::candidate& c)
{
    // DEBT: A bit sloppy converting bundles like this, but gets the job done
    move(get_bundle(c.bundle.handle), get_bundle(c.move_to.handle), 0, c.overlap);
}

template <class Traits>
template <class HandleTraits>
void pool<Traits>::ops<HandleTraits>::assess(fragmentation* frag) const
{
    pos_type largest_free_sz{0};

    const_bundle cur = first();
    // There is no prev at the very first block
    // DEBT: 'next' if cur->has_next() is false creates a UB/invalid pointer for 'block'.  Doesn't violate
    // our internal rules but extremely easy to stumble over
    const_bundle bn_prev{}, bn_next = next(cur);
    int last_sc = 0;

    fragmentation::candidate& top = frag->candidates[0];
    top = {};
    frag->largest_free_handle = -1;
    frag->candidates[1] = {};

    static constexpr uint16_t booster = 4;

    pos_type bn_cur_sz{0};

    while(cur.handle != block::null)
    {
        pos_type bn_prev_sz = bn_cur_sz;
        bn_cur_sz = phys_size(cur);

        // DEBT: I don't think there are any invariant conditions in which 'cur' is_null.  Document if so.

        if(cur.is_null() == false && cur.allocated() == false)
        {
            if(bn_cur_sz > largest_free_sz)
            {
                largest_free_sz = bn_cur_sz;
                frag->largest_free_handle = cur.handle;
            }
        }

        if(cur.is_null() == false && bn_prev.is_null() == false && cur.has_next())
        {
            pos_type bn_next_sz = phys_size(bn_next);

            // F A F
            if(!bn_prev.allocated() && cur.allocated() && !bn_next.allocated())
            {
                //int sc = score(bn_prev, cur, bn_next);
                pos_type v(0);
                const_bundle* which = &bn_prev;
                bool overlap = false;

                // favor trivial
                // favor a triple with a small middle, since that's easier to move
                // TODO: Eventually favor one which doesn't require an overlap, since less
                // maintenance involved.  Not enabling yet because it's convenient to poke the bear
                // and have more overlaps

                if(cur.is_trivial())
                {
                    pos_type max = bn_prev_sz + bn_next_sz;

                    pos_type prev_boost = (max - bn_prev_sz) * booster;
                    pos_type next_boost = (max - bn_next_sz) * booster;

                    // If following F block is the would-be end big-free-block, greatly favor
                    // previous F block so that merge can create end big-free-block
                    if(!bn_next.has_next()) prev_boost *= booster * booster;

                    // Favor the smaller fitting F
                    if(next_boost > prev_boost)
                    {
                        overlap = bn_cur_sz > bn_next_sz;
                        which = &bn_next;
                    }
                    else
                        overlap = bn_cur_sz > bn_prev_sz;

                    v += prev_boost + bn_cur_sz + next_boost;
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
                    top = { sc, cur, *which, overlap };
                    last_sc = sc;
                }
            }

// If this is actually A A F A or A F A A this can make fragmentation worse, so needs attention.  Also has
// some other glitch which seems to cause data corruption.  Temporarily disabled
#if UNUSED
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
#endif
        }

        bn_prev = cur;
        cur = bn_next;

        // DEBT: A little sloppy
        if(cur.handle != block::null)  next(cur.block, &bn_next);
    }
}

}}

}}
