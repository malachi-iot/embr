#pragma once

#include "../udp.h"

namespace embr { namespace lwip { namespace internal {

struct Pbuf
{
    typedef struct pbuf value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;

protected:
    pointer p;

    ESTD_CPP_DEFAULT_CTOR(Pbuf)
    Pbuf(pointer p) : p(p) {}

public:
#ifdef FEATURE_CPP_DECLTYPE
    typedef decltype(p->len) size_type;
#else
    typedef uint16_t size_type;
#endif

    // DEBT: Likely we need to consolidate these, given the bitwise
    // nature of constness
    const_pointer pbuf() const { return p; }
    pointer pbuf() { return p; }
};

}}}