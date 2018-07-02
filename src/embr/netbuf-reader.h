#pragma once

#include <estd/span.h>
#include "netbuf.h"

namespace embr { namespace mem {

template <class TNetBuf>
class NetBufReader : internal::NetBufWrapper<TNetBuf>
{
    TNetBuf m_netbuf;

    typedef internal::NetBufWrapper<TNetBuf> base;
    typedef typename base::netbuf_type netbuf_type;
    typedef typename base::size_type size_type;

public:

protected:
    // represents how many bytes we've already read
    // note that this value has only indirect meaning in relation to 'total_size'
    // it primarily describes movement through current netbuf.data()
    size_type m_pos;

    const netbuf_type& netbuf() const { return m_netbuf; }

public:
    NetBufReader() : m_pos(0) {}

#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    NetBufReader(TArgs&&...args) :
        m_netbuf(std::forward<TArgs>(args)...),
        m_pos(0)
    {}
#endif

    estd::const_buffer buffer() const
    {
        return estd::const_buffer(netbuf().data() + m_pos, netbuf().size());
    }

    size_type total_size() const { return netbuf().total_size(); }

    bool end() const { return netbuf().end(); }

    bool advance(size_type by_amount)
    {
        m_pos += by_amount;
    }
};

}}
