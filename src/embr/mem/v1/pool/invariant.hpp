#include <estd/internal/rtto.h>
#include <estd/new.h>
#include <estd/numeric.h>

#if FEATURE_STD_OSTREAM
#include <iostream>
#endif

#include "../block.hpp"
#include "../pool.h"

namespace embr { namespace mem {

namespace detail { inline namespace v1 {

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
    const unsigned max_handles = handles_.size();

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
        return result({"no handles", ""});

    // Now, walk forward and check that 'next' is sane
    const_bundle bn = get_bundle(*first);
    const_bundle bn_last{};
    pos_type size_tally{0};
    unsigned handle_count = 0;

    for(; bn.handle != null; ++handle_count, next(bn.block, &bn), bn)
    {
        EMBR_MEM_INVARIANT_ASSERT(handle_count < max_handles, "circular list detected", "during next check");

        // As we walk forward, if physical position moves backward, that's an error
        if(!bn_last.is_null())
            if(bn.pos() < bn_last.pos())
                return result({"position check failed", "during next check"});

        if(bn.page->is_null())
            return result({"null page encountered", "during next check"});

        size_tally += phys_size(bn);

        bn_last = bn;
    }

    if(size_tally != size)
        return result({"size tally failed", "during next check"});

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
std::ostream& pool<Traits>::ops<HandleTraits>::dump(std::ostream& out) const
{
    using bytes_type = estd::units::v1::detail::unit<page_unit_traits<unsigned, estd::ratio<1>>>;
    constexpr handle_type null = traits::null;
    constexpr pos_type zero_pos = pos_type(0);
    const page_type* first{};
    const unsigned handles_size = handles_.size();

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
        return out;
    }

    int counter = 0;

    for(const_bundle bn = get_bundle(*first); bn.handle != null; ++counter, next(bn.block, &bn), bn)
    {
        if(counter == handles_size)
        {
            out << "exceeded " << handles_size << " bundles, aborting\n";
            return out;
        }

        out << "Bundle: handle=" << (int)bn.handle;
        if(bn.page->is_null())
        {
            out << " null page - abort\n";
            break;
        }
        bytes_type sz = phys_size(bn);
        out << ", " << (bn.allocated() ? "A" : "F");
        out << (bn.block->mode() == v1::block::Trivial ? 'T' : 'N');
        out << ", prev=" << (int)bn.block->prev();
        out << ", next=" << (int)bn.block->next();
        out << ", sz=" << sz.count() << "b";
        out << ", pos=" << bytes_type(bn.pos()).count() << "b";
        //out << ", block=" << bn.block;
        out << '\n';
    }

    out.flush();
    return out;
}
#endif

}}

}}
