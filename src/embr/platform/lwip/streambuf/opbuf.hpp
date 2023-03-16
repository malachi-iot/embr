#pragma once

#include "opbuf.h"

namespace embr { namespace lwip {

#ifdef FEATURE_CPP_INLINE_NAMESPACE
inline
#endif
namespace upgrading { namespace impl {

// Placing non-inline because it's kinda bulky
template <class TCharTraits, unsigned grow_by>
typename TCharTraits::int_type opbuf_streambuf<TCharTraits, grow_by>::overflow(int_type ch)
{
    // If there's no buffer space left to write to
    if(xout_avail() == 0)
    {
        // Attempt to move to the next allocated pbuf.  Probably there isn't
        // one
        if(!this->move_next())
        {
            // Lightly tested
            // DEBT: Not 100% convinced this 'grow_by' template value is the way
            // to go, but it vastly beats hardcoding
            v1::PbufBase appended(grow_by);

            // TODO: Might want to check appended.valid() to be sure, though
            // pretty sure concat of a null pbuf will yield similar results in the end

            this->pbuf_current.concat(appended);

            // Now move to this next pbuf and also test
            // to see if concat actually worked
            if(!this->move_next())
                return traits_type::eof();
        }

        // it's presumed that next buf in pbuf chain can fit at least one character
    }

    if(ch != traits_type::eof())
    {
        *pptr() = ch;
        // DEBT: pbump'ing here and not sputc feels odd.
        this->pbump(1);
    }

    // DEBT: We can do better than this.  Can't return ch since sometimes it's eof
    // even when we do have more buffer space.  That said this is technically a valid
    // success code:
    // Guidance: https://en.cppreference.com/w/cpp/io/basic_streambuf/overflow
    // "Returns unspecified value not equal to Traits::eof() on success"
    return traits_type::eof() - 1;
}

}}

}}