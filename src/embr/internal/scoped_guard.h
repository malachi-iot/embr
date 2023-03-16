#pragma once

#include <estd/type_traits.h>
#include <estd/variant.h>

#include "fwd.h"

namespace embr { namespace internal {

enum scoped_guard_fail_action
{
    SCOPED_GUARD_ASSERT = 0,

    // These two retain status
    SCOPED_GUARD_WARN = 1,
    SCOPED_GUARD_SILENT = 2
};

// DEBT: I've done something like this elsewhere, but this naming matches
// std c++11 fairly well so cascade this out to estd
// See: https://en.cppreference.com/w/cpp/thread/scoped_lock for similar naming
// All that said, something like 'unique_value' would match the expected behavior
// better, so maybe consider that (ala unique_ptr).

// Part of the naming difficulty
// is we are defining RIAA behavior first, and memory sharing/allocation 2nd.
// Therefore, our use cases include both things like esp-idf nvs handle AND
// LwIP pbuf - the latter having rather sophisticated memory management.
// For this reason, we may settle on a more ambiguous name, like merely "scoped"

// DEBT: If this all works really well, consider moving to estd

// NOTE: "Scope Guard" tends to mean do special things on scope exit only,
// not so much on scope entry.  However, it appears it's not a formal term: 
// https://stackoverflow.com/questions/31365013/what-is-scopeguard-in-c
// https://github.com/Neargye/scope_guard
// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Scope_Guard
// https://ricab.github.io/scope_guard/

// NOTE: Using scoped_guard implies an assert/exception style behavior - expect
// the system to halt if ctor/dtor is not fully successful
template <class T, scoped_guard_fail_action = SCOPED_GUARD_WARN>
class scoped_guard;

template <class T>
struct scoped_status_traits
{
    ESTD_CPP_CONSTEXPR_RET static bool good(T) { return true; }
    ESTD_CPP_CONSTEXPR_RET static void log(T) {}
    ESTD_CPP_CONSTEXPR_RET static void assert_(T) {}
};


template <class T>
struct scoped_guard_traits
{
    typedef estd::monostate status_type;
};



template <class T, bool with_status, scoped_guard_fail_action action>
class scoped_guard_base;


// DEBT: I could swear I did one of these for estd already
#define EMBR_CPP_STANDARD_TYPEDEF(T)            \
    typedef T value_type;                       \
    typedef value_type& reference;              \
    typedef const value_type& const_reference;  \
    typedef value_type* pointer;                \
    typedef const value_type* const_pointer;



template <class T, scoped_guard_fail_action action>
class scoped_guard_base<T, false, action>
{
public:
    EMBR_CPP_STANDARD_TYPEDEF(T)

    typedef scoped_guard_traits<T> guard_traits;
    typedef typename guard_traits::status_type status_type;
    typedef scoped_status_traits<status_type> status_traits;

protected:
    value_type value_;

    reference value() { return value_; }
    ESTD_CPP_CONSTEXPR_RET const_reference value() const { return value_; }

    void status(status_type s)
    {
        switch(action)
        {
            case SCOPED_GUARD_ASSERT:
                status_traits::assert_(s);
                break;

            case SCOPED_GUARD_WARN:
                status_traits::log(s);
                break;

            // implicit action is silent
            default:    break;
        }
    }

    ESTD_CPP_DEFAULT_CTOR(scoped_guard_base)

//#if __cpp_variadic_templates
//#else
    template <class T1>
    ESTD_CPP_CONSTEXPR_RET scoped_guard_base(T1 p1) :
        value_(p1)
    {}
//#endif

public:
    //operator reference() { return value_; }
    //operator const_reference() const { return value_; }
    ESTD_CPP_CONSTEXPR_RET const_pointer get() const { return &value_; }

    ESTD_CPP_CONSTEXPR_RET const_pointer operator*() { return get(); }
    pointer operator->() { return &value_; }
    ESTD_CPP_CONSTEXPR_RET const_pointer operator->() const { return &value_; }

    // Success is always implied when no status is specifically tracked
    constexpr bool good() const { return true; }
};

template <class T, scoped_guard_fail_action action>
class scoped_guard_base<T, true, action> : public scoped_guard_base<T, false, action>
{
    typedef scoped_guard_base<T, false, action> base_type;

public:
    typedef typename base_type::guard_traits guard_traits;
    typedef typename guard_traits::status_type status_type;
    typedef typename base_type::status_traits status_traits;

protected:
    status_type status_;

    void status(status_type s)
    {
        status_ = s;
        base_type::status(s);
    }

public:
    constexpr status_type status() const { return status_; }
    constexpr bool good() const { return status_traits::good(status_); }
};



}}