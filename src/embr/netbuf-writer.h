#pragma once

#include <estd/span.h>
#include "netbuf.h"

#ifdef FEATURE_C_ITOA
#include <stdlib.h>
#endif


namespace embr { namespace mem {


// TODO: writer is potentially going to need all that ostream stuff
// so keep << unfancy until we finish porting in util.embedded ostream, then make
// NetBufWriter an actual participant in that space
template <class TNetBuf>
class NetBufWriter : public internal::NetBufWrapper<TNetBuf>
{
    typedef internal::NetBufWrapper<TNetBuf> base;

    template <class _TNetBuf>
    friend NetBufWriter<_TNetBuf>& operator <<(NetBufWriter<_TNetBuf>& reader, uint8_t value);

public:
    typedef typename base::netbuf_type netbuf_type;
    typedef typename base::size_type size_type;

protected:
    netbuf_type& netbuf() { return base::m_netbuf; }

public:
#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    NetBufWriter(TArgs&&...args) :
        base(std::forward<TArgs>(args)...)
    {}
#endif

    estd::mutable_buffer buffer()
    {
        uint8_t* data = netbuf().data();
        return estd::mutable_buffer(data + base::m_pos,
                                  netbuf().size() - base::m_pos);
    }

    // always attempts to move to a new chain (if chaining is used) with requested
    // memory signature.  If ot chained, a regular realloc/expand is attempted
    // TODO: next (expand) should really return a pass, fail or partial pass
    // if some but not all memory could be allocated
    bool next(size_type by_amount)
    {
        switch(netbuf().expand(by_amount, true))
        {
            case ExpandResult::ExpandOKChained:
                base::m_pos = 0;

            case ExpandResult::ExpandOKLinear:
                return true;

            case ExpandResult::ExpandFailFixedSize:
            case ExpandResult::ExpandFailOutOfMemory:
            default:
                return false;
        }
    }

    bool write(const estd::const_buffer& b)
    {
    }
};


template <class TNetBuf>
NetBufWriter<TNetBuf>& operator <<(NetBufWriter<TNetBuf>& writer, const estd::const_buffer& copy_from)
{
    estd::mutable_buffer b = writer.buffer();
    typedef typename NetBufWriter<TNetBuf>::size_type size_type;

    /*
    if(copy_from.size() > b.size())
    {
        typedef estd::const_buffer::iterator iterator;
        size_type remainder;
        size_type copy_from_size = copy_from.size();
        iterator copy_from_begin = copy_from.begin();

        do
        {
            // remainder = amount we still need to write
            remainder = copy_from_size - b.size();

            // fill remainder of current chunk up completely
            std::copy(copy_from_begin, copy_from_begin + b.size(), b.begin());

            // advance to next chunk
            if (!writer.next(remainder))
            {
                // ASSERT some kind of problem
            }

            copy_from_begin += remainder;
            copy_from_size -= remainder;

            b = writer.buffer();

            // keep going if copy_from still is larger than the presented chunk
        } while(copy_from_size > b.size());
    }
    else */
        std::copy(copy_from.begin(), copy_from.end(), b.begin());

    return writer;
}

template <class TNetBuf>
NetBufWriter<TNetBuf>& operator <<(NetBufWriter<TNetBuf>& writer, uint8_t value)
{
    writer.netbuf().data()[0] = value;
    return writer;
}

namespace experimental {

template <class TNetBuf>
void itoa(NetBufWriter<TNetBuf>& writer, int value, int base = 10)
{
    estd::mutable_buffer b = writer.buffer();
#ifdef FEATURE_C_ITOA
    // FIX: untested
    itoa(value, (char*)b.data(), base);
#else
    if(base == 10)
    {
        // TODO: Check for size of int string, ideally we'd account for:
        // - char_traits for unicode
        // - maximum size of this platform's int value (which I believe some helpers are floating somewhere in estd)
        // - of course we have to account for discarded null termination
        int advance_by = sprintf((char*) b.data(), "%d", value);
        writer.advance(advance_by);
    }
#endif
}

}

}}
