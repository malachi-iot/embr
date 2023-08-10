#pragma once

#include "../../internal/debounce/ultimate.h"

namespace embr { namespace arduino { namespace debounce {

inline namespace v1 { inline namespace ultimate {

template <unsigned pin, bool inverted = false>
struct Debouncer : embr::debounce::Tracker<uint16_t, inverted>
{
    using base_type = embr::debounce::Tracker<uint16_t, inverted>;

    bool eval()
    {
        return base_type::eval(digitalRead(pin));
    }
};

// Semi-experimental - if you really want it, you can extract I
// to get at tuple index
template <unsigned I>
using Event = embr::debounce::v1::Event;


struct Visitor
{
    template <unsigned I, unsigned pin, class F>
    bool operator()(estd::variadic::instance<I, Debouncer<pin>> i, F&& f) const
    {
        if(i->eval())   f(Event<I>{i->state(), pin});
        return false;
    }
};




}}

}}}