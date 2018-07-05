#pragma once

#include <estd/span.h>
//#include <estd/ios.h>

/*
 * EXPAND operation can optionally auto-move to next chain if it's a chaining netbuf
 *
 * NetBufReader treats NetBuf.size() as maximum readable (processed) bytes,
 *      and tracks its own pos reading through it.  It is assumed that every byte presented
 *      by NetBuf is interesting and readable - nothing uninitialized
 * NetBufWriter treats NetBuf.size() as maximum writeable (unprocessed) bytes,
 *      and tracks its own pos filling it up so that subsequent buffer() calls always
 *      present empty, writeable buffer space
 * NetBuf signature (concept) is:
 *      uint8* data() = raw available bytes in this chunk/chain
 *      size_type size() = number of available bytes in chunk/chain
 *      expand(size_type by_amount, bool auto_next) = attempt to expand netbuf size
 *      bool last() = check to see if this is the last nextbuf
 *      bool next() = attempt to move forward in a netbuf chain (only relevant for chained, prepopulated netbuf)
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


namespace experimental {

// for things like ios formatter and error bits
template <bool has_metadata>
class NetBufMetadata;

/*
template <>
struct NetBufMetadata<true> : estd::ios_base {}; */

template <>
struct NetBufMetadata<false> {};


}

namespace internal {


// since read and write are 90% similar, consolidate their functions here
template <class TNetBuf>
class NetBufWrapper : experimental::NetBufMetadata<false>
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

    // TODO: Revisit this.  Named it 'last' so it wouldn't collide with inherited
    // 'end' for static netbuf but realizing array has a 'last' call too...
    bool last() const { return m_netbuf.last(); }

    size_type advance(size_type by_amount)
    {
        if(m_netbuf.size() < by_amount)
            by_amount = m_netbuf.size();

        m_pos += by_amount;

        return by_amount;
    }
};

}

}}
