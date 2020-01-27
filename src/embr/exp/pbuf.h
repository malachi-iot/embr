/**
 * @file
 */
#pragma once

#include <estd/span.h>

namespace embr { namespace experimental {

template <class TAddr>
struct address_traits
{
    // special compare which only compares pure addresses and excludes things like port number
    static bool equals_fromto(const TAddr& from_addr, const TAddr& to_addr)
    {
        return from_addr == to_addr;
    }
};


// empty class, designed to be specialized
// remember our embr netbufs are specifically:
// a) underlying system-specific buffer - possibly chained
// b) tracks which chain we are on. pbuf itself does not track this
// also, it seems it would be convenient to have a pbuf-cpp wrapper which
// interacted with move semantics so as to behave a tiny bit like a shared_ptr
template <class TPBuf>
struct pbuf_traits
{
    typedef TPBuf pbuf_type;

    // NOTE: Not sure if we can utilize this 'use'/'unuse' paradigm with
    // shared_ptr
    static void use(pbuf_type&); // reference inc
    static void unuse(pbuf_type&); // reference dec.  If reaching 0, it is expected pbuf self-deallocates

    // gets the first (and maybe only) portion of a pbuf
    static estd::mutable_buffer raw(pbuf_type&);

    // same as existing embr netbuf type, but in theory we'd be tracking pbufs and
    // netbufs would become incidental
    typedef int netbuf_type;

    // would lean heavily on RVO, created netbuf is expected to be incidental
    static netbuf_type create_netbuf(pbuf_type&);
};




}}