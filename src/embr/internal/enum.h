#pragma once

namespace embr { namespace internal {

enum msg_bipbuf_options
{
    MBB_OPT_NONE            = 0x00,
    MBB_OPT_UNALIGNED       = 0x01,

    /// By default, msg_bipbuf is free to take action on underlying message size without retaining
    /// original size necessarily.  This flag ensures that original size really is retained.
    MBB_OPT_PRECISE_SIZE    = 0x02
};

}}
