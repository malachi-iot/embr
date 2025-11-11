#include "v2/enum.h"

#define CASE(s) case detail::service::s: return #s;

namespace embr { namespace service { namespace v2 {

const char* to_string(detail::service::states v)
{
    switch(v)
    {
        CASE(Stopped)
        CASE(Started)
        CASE(Error)
        default:    return "Unknown";
    }
}

const char* to_string(detail::service::substates v)
{
    switch(v)
    {
        CASE(Unstarted)
        CASE(Configuring)
        CASE(Configured)
        CASE(Finished)
        CASE(Restarting)
        CASE(Starting)
        CASE(Sleeping)
        CASE(Paused)
        CASE(Resuming)
        CASE(Waking)
        CASE(Dependency)

        CASE(Running)
        CASE(Connecting)
        CASE(Online)
        CASE(Disconnecting)
        CASE(Offline)
        CASE(Degraded)
        CASE(Pausing)
        CASE(Stopping)
        CASE(Resetting)
        CASE(DozingOff)
        CASE(Sparse)

        CASE(ErrUnspecified)
        CASE(ErrConfig)
        CASE(ErrMemory)
        CASE(ErrTimeout)

        default:    return "Unknown";
    }
}


}}}
