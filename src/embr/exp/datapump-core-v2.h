/**
 * @file
 */
#pragma once

#include "pbuf.h"

namespace embr { namespace experimental {

struct empty {};

// datapump-specific portion of its item.  specified out here mainly so
// retry logic can pass it around a bit more easily
template <class TPBuf, class TAddr>
struct Datapump2CoreItem
{
    // TODO: eventually clean up and use all those forward_node helpers
    Datapump2CoreItem* _next;

    // node_traits will automatically forward cast for us.  Generally accepted
    // practice, but be careful!
    void* next() { return _next; }
    void next(Datapump2CoreItem* n) { _next = n; }

    // major difference between PBUF and netbuf is PBUF state is not expected to change
    // during internal datapump operations, whereas netbuf have positioning data
    TPBuf pbuf;
    TAddr addr;

    typedef typename estd::remove_reference<TPBuf>::type pbuf_type;
    typedef TAddr addr_type;
};



}}