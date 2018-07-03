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

    // TODO: do bounds checking
    std::copy(copy_from.begin(), copy_from.end(), b.begin());
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
