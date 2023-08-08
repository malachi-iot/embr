#include "debounce/reference.h"

namespace embr {

// DEBT: Need to put in embr proper - but placing into debounce.cpp makes esp mad,
// creates a compatibility conflict with old gptimer
const char* to_string(debounce::v1::States s)
{
    switch(s)
    {
        case embr::debounce::States::Pressed:    return "pressed";
        case embr::debounce::States::Released:   return "released";
        default:                return "undefined";
    }
}

}