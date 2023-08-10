#pragma once

#include "../../internal/debounce/ultimate.h"

namespace embr { namespace arduino {

// DEBT: May be higher precision for some platforms
// DEBT: Move this elsewhere, or if Arduino has something like this typedef
// onhand already use that instead
using pin_type = uint8_t;    
    
namespace debounce {

inline namespace v1 { inline namespace ultimate {

template <pin_type pin, bool inverted = false>
struct Debouncer : embr::debounce::Tracker<uint16_t, inverted>
{
    using base_type = embr::debounce::Tracker<uint16_t, inverted>;

    bool eval()
    {
        return base_type::eval(digitalRead(pin));
    }
};

template <>
struct Debouncer<(pin_type)-1>
{
    // DEBT: Needed because visit_tuple_functor doesn't fully play nice with
    // sparse tuple just yet
    estd::byte dummy;

    static constexpr bool eval() { return false; }

    static constexpr embr::debounce::v1::States state()
    {
        return embr::debounce::v1::States::Undefined;
    }
};


// Semi-experimental - if you really want it, you can extract I
// to get at tuple index
template <unsigned I>
struct Event : embr::debounce::v1::Event
{
    using base_type = embr::debounce::v1::Event;

    constexpr Event(embr::debounce::v1::States s, pin_type pin) :
        base_type{s, pin} {}
};


struct Visitor
{
    template <unsigned I, pin_type pin, class F>
    bool operator()(estd::variadic::instance<I, Debouncer<pin>> i, F&& f) const
    {
        if(i->eval())   f(Event<I>{i->state(), pin});
        return false;
    }
};




}}

}}}