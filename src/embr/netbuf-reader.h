#pragma once

#include <estd/span.h>
#include "netbuf.h"

namespace embr { namespace mem {

template <class TNetBuf>
class NetBufReader : internal::NetBufWrapper<TNetBuf>
{
    typedef internal::NetBufWrapper<TNetBuf> base;

public:
    typedef typename base::netbuf_type netbuf_type;
    typedef typename base::size_type size_type;

protected:
    const netbuf_type& netbuf() const { return base::m_netbuf; }

public:
#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    NetBufReader(TArgs&&...args) :
        base(std::forward<TArgs>(args)...)
    {}
#endif

    estd::const_buffer buffer() const
    {
        return estd::const_buffer(netbuf().data() + base::m_pos,
                                  netbuf().size());
    }

    bool advance(size_type by_amount)
    {
        base::m_pos += by_amount;
        return true;
    }
};

}}
