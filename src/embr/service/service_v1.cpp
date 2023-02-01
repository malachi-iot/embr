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
        case Starting:      return "Starting";
        case Stopping:      return "Stopping";
        case Dependency:    return "Dependency";
        case Unstarted:     return "Unstarted";
        case Sleeping:      return "Sleeping";
        case Connecting:    return "Connecting";
        case Degraded:      return "Degraded";
        case Resetting:     return "Resetting";
        case Configuring:   return "Configuring";
        case Configured:    return "Configured";
        case Waking:        return "Waking";
        case Running:       return "Running";
        case Online:        return "Online";
        case Offline:       return "Offline";

        case ErrConfig:     return "Config Error";
        case ErrMemory:     return "Memory Error";
        case ErrUnspecified:    return "Unspecified Error";

        default:    return "Unknown";
    }
}

}}

}
