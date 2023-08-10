#pragma once

#include <estd/port/toolchain.h>    // Makes sure we get __GNUC_PREREQ

namespace embr { namespace debounce { inline namespace v1 {

inline namespace reference {

#if __GNUC_PREREQ(9, 3)
enum class States
#else
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=51242
enum class States : char
#endif
{
    Undefined = 0,
    On,
    Pressed = On,
    Off,
    Released = Off
};


// DEBT: Can't do const here because layer1::queue's array doesn't
// play nice with it.  Upgrade layer1::queue to use 'uninitialized_array' and
// filter by is_trivial, is_trivially_copyable or is_trvially_copy_assignable
struct Event
{
    States state : 8;
    unsigned pin : 8;
};

}

}}

const char* to_string(embr::debounce::v1::reference::States);

}
