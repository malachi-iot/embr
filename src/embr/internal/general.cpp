#include "debounce/reference.h"

namespace embr {

// DEBT: Use flash constant strings
const char* to_string(debounce::v1::States s)
{
    switch(s)
    {
        case debounce::States::On:    return "on";
        case debounce::States::Off:   return "off";
        default:                return "undefined";
    }
}

}