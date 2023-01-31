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

        default:    return "Unknown";
    }
}

}}

}
