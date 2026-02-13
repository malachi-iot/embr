#pragma once

#include <estd/internal/rtto.h>
#include <estd/new.h>
#include <estd/numeric.h>

#include "../block.hpp"
#include "../pool.h"

#include "move.hpp"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {



template <class Traits>
void pool_ops<Traits>::defrag(const typename fragmentation::candidate& c, bool relink)
{
    // DEBT: A bit sloppy converting bundles like this, but gets the job done
    bundle from(get_bundle(c.bundle.handle));
    bundle to(get_bundle(c.move_to.handle));

    move(from, to, 0, 0, c.overlap);

    if(relink)
    {
        // By definition move may change position of block, so re-acquire bundle
        // By definition move may change position of block and validity of handle, so re-acquire
        from = get_bundle(from.handle);
        to = get_bundle(to.handle);

        // It's entirely possible one of these handles got swallowed by GC, in which case although
        // we do need to relink, a swap is problematic.
        if(from.page->is_null())
            virtual_move(to, from);
        else if(to.page->is_null())
            virtual_move(from, to);
        else
            virtual_swap(from, to);
    }
}

template <class Traits>
void pool_ops<Traits>::assess(fragmentation* frag) const
{
    static constexpr pos_type zero(0);

    using pos_traits = typename pos_type::traits;
    // estd v0.8.11-beta2+ feature
    using ipos = estd::units::detail::unit<typename pos_traits::template rebind<int>>;
    pos_type largest_free_sz{0};

    const_bundle cur = first();
    // There is no prev at the very first block
    // DEBT: 'next' if cur->has_next() is false creates a UB/invalid pointer for 'block'.  Doesn't violate
    // our internal rules but extremely easy to stumble over
    const_bundle bn_prev{}, bn_next = next(cur);
    int last_sc = 0;

    typename fragmentation::candidate& top = frag->candidates[0];
    top = {};
    frag->largest_free_handle = -1;
    frag->candidates[1] = {};

    static constexpr uint16_t booster = 4;

    pos_type bn_cur_sz{0};

    while(cur.handle != block::null)
    {
        assert(!cur.is_null());

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
                block::metadata_type metadata = cur.block->metadata();

                // favor trivial
                // favor a triple with a small middle, since that's easier to move
                // TODO: Eventually favor one which doesn't require an overlap, since less
                // maintenance involved.  Not enabling yet because it's convenient to poke the bear
                // and have more overlaps

                if(cur.is_trivial())
                {
                    pos_type max = bn_prev_sz + bn_next_sz;

                    pos_type prev_boost((max - bn_prev_sz) * booster);
                    pos_type next_boost((max - bn_next_sz) * booster);

                    // If following F block is the would-be end big-free-block, favor
                    // previous F block so that merge can create end big-free-block
                    if(!bn_next.has_next()) prev_boost *= booster;

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
#if FEATURE_ESTD_RTTO_GET_METADATA
                else if(metadata->moveable)
#else
                else
#endif
                {
                    // Non-trivial calculated differently, since overlap is not permitted
                    ipos bn_prev_delta = bn_prev_sz - bn_cur_sz;
                    ipos bn_next_delta = bn_next_sz - bn_cur_sz;

                    if(bn_prev_delta >= zero)
                        v += bn_prev_sz;

                    if(bn_next_delta >= zero)
                    {
                        // If we're a tighter fit than prev OR prev never was a candidate,
                        // then choose next
                        if(bn_next_delta < bn_prev_delta || v == zero)
                            which = &bn_next;

                        v += bn_next_sz;
                    }
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
