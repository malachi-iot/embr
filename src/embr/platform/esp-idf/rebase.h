// Rebaser to ensure scheduler doesn't overflow
// NOTE: Goal is portability - in the short term we'll put this in esp-idf category
#pragma once

#include <estd/chrono.h>
#include <estd/type_traits.h>


// DEBT: Move this elsewhere
#define EMBR_CPP_VALUE_TYPE(T) \
typedef T value_type;           \
typedef value_type& reference;  \
typedef const value_type& const_reference; \
typedef T* pointer;


namespace embr { namespace internal {

template <class T, typename = void>
struct rebase_traits
{
    EMBR_CPP_VALUE_TYPE(T)

    typedef typename value_type::time_point duration;

    inline static void rebase(reference v, duration t) { v.rebase(t); }
};

template <class T, typename = void>
struct is_time_point : estd::false_type {};

template <class Clock, class Duration>
struct is_time_point<estd::chrono::time_point<Clock, Duration> > : estd::true_type {};

template <class Clock, class Duration>
struct is_time_point<std::chrono::time_point<Clock, Duration> > : estd::true_type {};

template <class T>
struct rebase_traits<T,
    typename estd::enable_if<is_time_point<typename T::time_point>::value>::type >
{
    EMBR_CPP_VALUE_TYPE(T)

    typedef typename value_type::time_point::duration duration;

    inline static void rebase(reference v, duration t) { v.rebase(t); }
};




template <class TContainer, class TTraits = rebase_traits<typename TContainer::value_type> >
class Rebaser
{
    typedef TContainer container_type;
    typedef typename container_type::iterator iterator;

    typedef typename container_type::value_type value_type;
    typedef TTraits traits_type;
    typedef typename traits_type::duration duration;

    container_type& container_;

    // Just keeping around in case we still want the more fluid template
    // flavor of this - and doesn't hurt code behavior
    template <typename TDuration>
    void rebase_internal(TDuration d) const
    {
        for(iterator i = container_.begin(); i != container_.end(); ++i)
        {
            value_type& v = *i;

            traits_type::rebase(v, d);
        }
    }

public:
    Rebaser(container_type& container) : container_(container)
    {

    }

    /// Subtracts duration 'd' from all event_due() in container
    void rebase(duration d) const { rebase_internal(d); }
};

}}