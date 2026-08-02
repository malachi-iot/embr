#pragma once

#include "../../mem/v1/unique-handle.h"
#include "../../mem/v1/functional/list.h"

#include "changed.h"

namespace embr { inline namespace property { inline namespace v2 {

// NOT READY YET
template <class Pool, Pool* pool>
class provider
{
    using funclist_type = mem::v1::funclist<void(const changed<>*), Pool, pool>;

protected:
    funclist_type changed_;

    template <const char* prop, class Getter, class Setter, class T>
    void set_fn(Getter&& g, Setter&& s, const T& value)
    {
        s(value);

        changed<prop> c;

        //changed_.invoke(c);
    }

    template <const char* prop, class Dest, class T>
    void update(Dest& dest, const T& value)
    {
        set_fn<prop>(
            [&] { return dest; },
            [&](const T& v) { dest = v; },
            value);
    }

public:
    using value_type = typename funclist_type::value_type;

    // DEBT: Open-ended initializer like this not great, in_place_t and similar may be
    // interesting
    template <class ...Args>
    constexpr provider(Args&&...args) : changed_{std::forward<Args>(args)...} {}

    template <class F>
    value_type on_changed(F&& f)
    {
        return changed_.push_back(std::forward<F>(f));
    }

    void remove(value_type v)
    {
        changed_ -= v;
    }
};

}}}

