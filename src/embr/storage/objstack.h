#pragma once

#include <estd/array.h>
#include <estd/span.h>

#include "fwd.h"

namespace embr { namespace detail { inline namespace v1 {

template <int alignment_, class Int>
static constexpr Int align(Int v)
{
    return ((v + ((1 << alignment_) - 1)) >> alignment_) << alignment_;
}

template <class Container, unsigned alignment = 2>
class objstack
{
    Container buffer_;
    char* current_;

public:
    using container_type = Container;

    static constexpr unsigned alignment_ = alignment;
    using size_type = unsigned;

    template <class ...Args>
    constexpr explicit objstack(Args&&...args) :
        buffer_(std::forward<Args>(args)...),
        current_{buffer_.data()}
    {

    }

    void* alloc(size_type sz)
    {
        sz = align<alignment_>(sz);

        const char* const end = limit();

        if(current_ + sz > limit()) return nullptr;

        void* allocated = current_;

        current_ += sz;

        return allocated;
    }

    // Remember, deallocs with objstack are particularly destructive!

    void dealloc(void* ptr)
    {
        current_ = (char*)ptr;
    }

    void dealloc(size_type sz)
    {
        current_ -= sz;
    }

    char* current() { return current_; }
    const char* current() const { return current_; }

    constexpr const char* limit() const { return buffer_.end(); }
};

}}}

namespace embr {

namespace layer1 { inline namespace v1 {

template <unsigned N>
using objstack = detail::objstack<estd::array<char, N>>;

}}

namespace layer2 { inline namespace v1 {

template <unsigned N>
using objstack = detail::objstack<estd::span<char, N>>;

}}

namespace layer3 { inline namespace v1 {

using objstack = detail::objstack<estd::span<char>>;

}}

}