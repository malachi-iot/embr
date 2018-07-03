#pragma once

#include <estd/span.h>
#include "netbuf.h"

namespace embr { namespace mem {

template <class TNetBuf>
class NetBufReader : public internal::NetBufWrapper<TNetBuf>
{
    typedef internal::NetBufWrapper<TNetBuf> base;

    template <class _TNetBuf>
    friend NetBufReader<_TNetBuf>& operator >>(NetBufReader<_TNetBuf>& reader, uint8_t& value);


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
                                  netbuf().size() - base::m_pos);
    }

    bool next() { return netbuf().next(); }
};

template <class TNetBuf>
NetBufReader<TNetBuf>& operator >>(NetBufReader<TNetBuf>& reader, uint8_t& value)
{
    value = reader.netbuf().data()[0];
    reader.advance(1);
    return reader;
}

}}
