/**
 *  @file
 *  12/10/2019: Generally one wants to use istream/ostream for this.  However those may or may not be
 *              C++11 dependent, so this NetBufReader may still have use for you
 */
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
    typedef estd::span<const uint8_t> const_buffer;

protected:
    const netbuf_type& netbuf() const { return base::m_netbuf; }

public:
#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    NetBufReader(TArgs&&...args) :
        base(std::forward<TArgs>(args)...)
    {}
#endif

    const_buffer buffer() const
    {
        return const_buffer(static_cast<const uint8_t*>(netbuf().data()) + base::m_pos,
                                  netbuf().size() - base::m_pos);
    }

    bool next() { return netbuf().next(); }
};

template <class TNetBuf>
NetBufReader<TNetBuf>& operator >>(NetBufReader<TNetBuf>& reader, uint8_t& value)
{
    const uint8_t* data = static_cast<const uint8_t*>(reader.netbuf().data());
    value = data[0];
    reader.advance(1);
    return reader;
}

}}
