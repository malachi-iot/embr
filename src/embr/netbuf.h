#pragma once

#include <estd/span.h>

namespace embr { namespace mem {

namespace internal {

// since read and write are 90% similar, consolidate their functions here
template <class TNetBuf>
class NetBufWrapper
{
protected:
public:
    typedef typename std::remove_reference<TNetBuf>::type netbuf_type;
    typedef typename netbuf_type::size_type size_type;
};

}

}}
