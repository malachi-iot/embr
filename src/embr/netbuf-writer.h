#pragma once

#include <estd/span.h>
#include "netbuf.h"

namespace embr { namespace mem {

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
                                  netbuf().size());
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

    bool advance(size_type by_amount)
    {
        base::m_pos += by_amount;
    }


    bool write(const estd::const_buffer& b)
    {
        // TODO: Do smart chain/expanding here
        memcpy(netbuf().data(), b.data(), b.size());
    }
};

}}
