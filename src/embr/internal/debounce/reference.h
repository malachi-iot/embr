#pragma once

namespace embr { namespace debounce { inline namespace v1 {

enum class States
{
    Undefined = 0,
    Pressed,
    Released
};


// DEBT: Can't do const here because layer1::queue's array doesn't
// play nice with it.  Upgrade layer1::queue to use 'uninitialized_array' and
// filter by is_trivial, is_trivially_copyable or is_trvially_copy_assignable
struct Event
{
    States state : 8;
    unsigned pin : 8;
};

}}

const char* to_string(embr::debounce::v1::States);

}