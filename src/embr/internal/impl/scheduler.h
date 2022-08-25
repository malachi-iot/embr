#pragma once

namespace embr { namespace internal {

struct fake_mutex
{
    void lock() {}

    void unlock() {}
};

namespace scheduler { namespace impl {

/// Reference scheduler item traits
/// \tparam T consider this the system + app data structure
template <class T>
struct Reference
{
    typedef T value_type;

    typedef estd::chrono::steady_clock::time_point time_point;

    /// Retrieve specialized wake up time from T
    /// \param value
    /// \return
    static time_point get_time_point(const T& value)
    {
        return value.event_due;
    }

    ///
    /// \param value
    /// \param current_time actual current time
    /// \return true = reschedule requested, false = one shot
    static bool process(T& value, time_point current_time)
    {
        return false;
    }

    struct mutex
    {
        inline void lock() {}

        inline void unlock() {}
    };
};

struct TraditionalBase
{
    typedef unsigned long time_point;

    struct control_structure
    {
        typedef bool (*handler_type)(control_structure* c, time_point current_time);

        time_point wake_time;
        handler_type handler;

        void* data;
    };

    typedef fake_mutex mutex;
};

template <bool is_inline>
struct Traditional;

template <>
struct Traditional<true> : TraditionalBase
{
    typedef control_structure value_type;

    static time_point get_time_point(const value_type& v) { return v.wake_time; }

    static bool process(value_type& v, time_point current_time)
    {
        return v.handler(&v, current_time);
    }
};

template <>
struct Traditional<false> : TraditionalBase
{
    typedef control_structure* value_type;

    static time_point get_time_point(value_type v) { return v->wake_time; }

    static bool process(value_type v, time_point current_time)
    {
        return v->handler(v, current_time);
    }
};


template <typename TTimePoint>
struct Function
{
    typedef TTimePoint time_point;
    typedef estd::detail::function<void(time_point*, time_point)> function_type;

    template <class F>
    static estd::experimental::inline_function<F, void(time_point*, time_point)> make_function(F&& f)
    {
        return estd::experimental::function<void(time_point*, time_point)>::make_inline2(std::move(f));
    }

    struct control_structure
    {
        time_point wake;

        function_type func;

        control_structure(time_point wake, function_type func) :
            wake(wake),
            func(func) {}

        // Don't give anybody false hope that we're gonna housekeep function_type
        control_structure(time_point wake, function_type&& func) = delete;

        // DEBT: See Item2Traits
        control_structure() = default;

        bool match(const function_type& f)
        {
            // DEBT: do operator overload for estd::detail::function itself,
            // although exposing a const model isn't totally terrible
            return func.getm() == f.getm();
        }
    };

    typedef control_structure value_type;

    static time_point get_time_point(const value_type& v) { return v.wake; }

    static bool process(value_type& v, time_point current_time)
    {
        time_point origin = v.wake;

        v.func(&v.wake, current_time);

        return origin != v.wake;
    }

    typedef fake_mutex mutex;
};

}}

}}