#pragma once

#include <estd/span.h>

/*
 * EXPAND operation can optionally auto-move to next chain if it's a chaining netbuf
 */

namespace embr { namespace mem {


template <class TNetBuf>
class NetBufTraits;

enum ExpandResult
{
    ExpandWarnChained = -4, // allocated *some* of requested memory
    ExpandWarnLinear = -3, // allocated *some* of requested memory

    ExpandFailFixedSize = -2,
    ExpandFailOutOfMemory = -1,
    // expanded by linking another chain in the list.  data() will
    // definitely be different here
    ExpandOKChained = 0,
    // expanded in-place, or perhaps realloc style
    // always query data() again just to be sure after the expand
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
