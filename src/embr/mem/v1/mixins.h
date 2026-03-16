#pragma once

#include <estd/internal/macro/cpp.h>
#include <estd/utility.h>

// DEBT: Put this guy up in estd
namespace embr { namespace mem { namespace mixins {

// Span-esque
template <class Derived, class T>
class container
{
public:
    ESTD_CPP_STD_VALUE_TYPE(T)

    using iterator = pointer;
    using const_iterator = const_pointer;

    reference operator[](int v)
    {
        return *(static_cast<Derived*>(this)->data() + v);
    }

    constexpr const_reference operator[](int v) const
    {
        return *(static_cast<const Derived*>(this)->data() + v);
    }

    pointer begin()
    {
        return static_cast<Derived*>(this)->data();
    }

    constexpr const_pointer cbegin() const
    {
        return static_cast<const Derived*>(this)->data();
    }

    constexpr const_pointer begin() const { return cbegin(); }

    pointer end()
    {
        auto self = static_cast<Derived*>(this);
        return self->data() + self->size();
    }

    constexpr const_pointer cend() const
    {
        auto self = static_cast<const Derived*>(this);
        return self->data() + self->size();
    }

    const_pointer end() const { return cend(); }

    reference front()
    {
        return *begin();
    }

    constexpr const_reference front() const
    {
        return *begin();
    }

    reference back()
    {
        return *(end() - 1);
    }

    constexpr const_reference back() const
    {
        return *(end() - 1);
    }

    constexpr bool empty() const
    {
        return static_cast<const Derived*>(this)->size() == 0;
    }
};


// This guy seems familiar, think we did this before
template <class Derived, class T>
class iterator_access
{
public:
    ESTD_CPP_STD_VALUE_TYPE(T)

private:

    pointer current()
    {
        return static_cast<Derived*>(this)->current_;
    }

    constexpr const_pointer current() const
    {
        return static_cast<const Derived*>(this)->current_;
    }

public:
    reference operator*()   { return *current(); }
    pointer operator->()   { return current(); }
};


// This guy seems familiar, think we did this before
template <class Derived>
class iterator_math
{
public:
    Derived& operator++()
    {
        auto self = static_cast<Derived*>(this);
        ++self->current_;
        return *self;
    }

    Derived operator++(int)
    {
        auto self = static_cast<Derived*>(this);
        Derived copy(*self);
        ++self->current_;
        return copy;
    }

    Derived& operator--()
    {
        auto self = static_cast<Derived*>(this);
        --self->current_;
        return *self;
    }

    Derived operator--(int)
    {
        auto self = static_cast<Derived*>(this);
        Derived copy(*self);
        --self->current_;
        return copy;
    }

    friend constexpr bool operator <(const Derived& lhs, const Derived& rhs)
    {
        return lhs.current_ < rhs.current_;
    }

    friend constexpr bool operator >(const Derived& lhs, const Derived& rhs)
    {
        return lhs.current_ > rhs.current_;
    }
};


template <class Derived, class T>
class iterator :
    public iterator_access<Derived, T>,
    public iterator_math<Derived>
{

};

// DEBT: I am warned by AI this doesn't cover a lot of cases
template <class T>
using dereference = decltype(*std::declval<T>());

template <class, class = void>
struct can_dereference : estd::false_type{};

template <class T>
struct can_dereference<T, estd::void_t<dereference<T>>> : estd::true_type {};

template <class Derived, class T, class Enabled = void>
class accessor_dereference {};

template <class Derived, class T>
class accessor_dereference<Derived, T, estd::enable_if_t<can_dereference<T>::value>>
{
    using dereferenced = dereference<T>;

public:
    ESTD_CPP_STD_VALUE_TYPE(T)

    // In these cases, reference itself is something like T*&
    reference operator->() { return static_cast<Derived*>(this)->value(); }
    constexpr const_reference operator->() const
    {
        return static_cast<const Derived*>(this)->value();
    }

    // UNTESTED
    dereferenced operator*() { return *operator->(); }
    const dereferenced operator*() const { return *operator->(); }
};

template <class Derived, class T>
class accessor_access : public accessor_dereference<Derived, T>
{
public:
    ESTD_CPP_STD_VALUE_TYPE(T)

    friend constexpr bool operator==(const_reference lhs, const Derived& rhs)
    {
        return lhs == rhs.value();
    }

    friend constexpr bool operator==(const Derived& lhs, const_reference rhs)
    {
        return lhs.value() == rhs;
    }

    constexpr const_reference operator()() const { return static_cast<const Derived*>(this)->value(); }
};


}}}

