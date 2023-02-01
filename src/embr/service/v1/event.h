#pragma once

namespace embr {

// DEBT: Fudging namespace here to 'property' so that we don't get ambiguous event namespace errors
inline namespace property { inline namespace v1 {

namespace event {

struct Registration
{
    const char* name;
    const char* instance;
};

struct Progress
{
    const unsigned percent;
    const char* comment;
};


}

}}

}