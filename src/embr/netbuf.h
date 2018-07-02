#pragma once

#include <estd/span.h>

namespace embr { namespace mem {

enum ExpandResult
{
    ExpandFailFixedSize = -2,
    ExpandFailOutOfMemory = -1,
    ExpandOKChained = 0,
    ExpandOKLinear = 1
};


namespace internal {

// since read and write are 90% similar, consolidate their functions here
template <class TNetBuf>
class NetBufWrapper
{
public:
    typedef typename std::remove_reference<TNetBuf>::type netbuf_type;
    typedef typename netbuf_type::size_type size_type;

protected:
    TNetBuf m_netbuf;

    // for reader:
    // represents how many bytes we've already read
    // note that this value has only indirect meaning in relation to 'total_size'
    // it primarily describes movement through current netbuf.data()
    // for writer:
    // represents how many bytes we've written
    // has same indirect relation to 'total_size'
    size_type m_pos;

#ifdef FEATURE_CPP_MOVESEMANTIC
    template <class ...TArgs>
    NetBufWrapper(TArgs&&...args) :
        m_netbuf(std::forward<TArgs>(args)...),
        m_pos(0)
    {}

    NetBufWrapper() = default;
#endif

public:
    size_type total_size() const { return m_netbuf.total_size(); }

    bool end() const { return m_netbuf.end(); }
};

}

}}
