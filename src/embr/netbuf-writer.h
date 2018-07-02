#pragma once

#include <estd/span.h>
#include "netbuf.h"

namespace embr { namespace mem {

template <class TNetBuf>
class NetBufWriter : internal::NetBufWrapper<TNetBuf>
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

    estd::const_buffer buffer()
    {
        return estd::const_buffer(netbuf().data() + base::m_pos,
                                  netbuf().size());
    }

    bool expand(size_type by_amount)
    {
        switch(netbuf().expand(by_amount))
        {
            case ExpandResult::ExpandOKChained:
                break;

            case ExpandResult::ExpandFailFixedSize:
            case ExpandResult::ExpandFailOutOfMemory:
                return false;
        }
    }

    bool advance(size_type by_amount)
    {
        base::m_pos += by_amount;
    }
};

}}
