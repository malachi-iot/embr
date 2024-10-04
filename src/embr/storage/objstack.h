#pragma once

#include "fwd.h"

namespace embr { namespace detail { inline namespace v1 {

// TODO: Can probably make all this happen with layer1::array etc

namespace layer1 { inline namespace v1 {

// DEBT: I think this guy would prefer to live straight under embr::layer1 -- this feels too far
// nested
template <unsigned N>
class objstack
{
    char buffer_[N];
    char* current_;

public:
    using size_type = unsigned;

    constexpr objstack() :
        current_{buffer_}
    {

    }

    void* alloc(size_type sz)
    {
        if(current_ + sz > limit()) return nullptr;

        void* allocated = current_;

        current_ += sz;

        return allocated;
    }

    // Remember, deallocs with objstack are particularly destructive!

    void dealloc(void* ptr)
    {
        current_ = ptr;
    }

    void dealloc(size_type sz)
    {
        current_ -= sz;
    }

    char* current() { return current_; }

    const char* limit() const { return buffer_ + N; }
};

}}

}}}
