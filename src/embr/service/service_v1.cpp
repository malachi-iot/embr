#include "v1/service.h"

namespace embr {

inline namespace service { inline namespace v1 {

const char* ServiceEnum::to_string(States v)
{
    switch(v)
    {
        case Stopped:       return "Stopped";
        case Started:       return "Started";
        case Error:         return "Error";
        default:    return "Unknown";
    }
}

const char* ServiceEnum::to_string(Substates v)
{
    switch (v)
    {
        case Configured:    return "Configured";
        case Configuring:   return "Configuring";
        case Connecting:    return "Connecting";
        case Degraded:      return "Degraded";
        case Dependency:    return "Dependency";
        case Disconnecting: return "Disconnecting";
        case Finished:      return "Finished";
        case Online:        return "Online";
        case Offline:       return "Offline";
        case Pausing:       return "Pausing";
        case Paused:        return "Paused";
        case Resuming:      return "Resuming";
        case Resetting:     return "Resetting";
        case Running:       return "Running";
        case Sleeping:      return "Sleeping";
        case Sparse:        return "Sparse";
        case Starting:      return "Starting";
        case Stopping:      return "Stopping";
        case Unstarted:     return "Unstarted";
        case Waking:        return "Waking";

        case ErrConfig:     return "Config Error";
        case ErrMemory:     return "Memory Error";
        case ErrTimeout:    return "Timeout Error";
        case ErrUnspecified:    return "Unspecified Error";

        default:    return "Unknown";
    }
}

}}

}
