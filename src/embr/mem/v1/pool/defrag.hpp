#include <estd/internal/rtto.h>
#include <estd/new.h>
#include <estd/numeric.h>

#include "../block.hpp"
#include "../pool.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

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
    const v1::block* b = self_.block(pos_type(0));
    static constexpr handle_type null = v1::block::null;

    if(b->next() == null)    return;

    const_bundle bn_prev{}, bn_next, cur;
    next(b, &bn_next);
    prev(bn_next.block, &cur);
    int last_sc = 0;

    fragmentation::candidate& top = frag->candidates[0];
    top = {};
    frag->candidates[1] = {};

    static constexpr uint16_t booster = 4;

    pos_type bn_cur_sz = phys_size(cur);

    while(cur.handle != block::null)
    {
        if(cur.is_null() == false && bn_prev.is_null() == false &&
            cur.block->next() != null)
        {
            pos_type bn_prev_sz = bn_cur_sz;
            pos_type bn_next_sz = phys_size(bn_next);
            bn_cur_sz = phys_size(cur);

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
                    pos_type prev_boost = bn_prev_sz * booster;
                    pos_type next_boost = bn_next_sz * booster;

                    // Favor the smaller fitting F
                    if(next_boost < prev_boost)
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
        next(cur.block, &bn_next);
    }
}

}}

}}
