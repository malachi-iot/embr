#pragma once

// DEBT: Would be better if we had a bip/fwd.h
#include <estd/internal/bip/buffer.h>

#include "enum.h"

namespace embr { namespace internal {

// 23MAY26 - Do we need to start considering a 'Traits' instead of options & align_to?
template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf,
    // NOTE: Defaulting align_to somewhat aggressively.  Technically this probably ought to be
    // alignof(std::max_align_t) but that can be pretty big.
    msg_bipbuf_options o = MBB_OPT_NONE, unsigned align_to = sizeof(unsigned)>
class msg_bipbuf;

template <class TupleLike, class Predicate = void>
class subject;

struct noop_mutex;

}}
