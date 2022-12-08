#pragma once

namespace embr { namespace internal {

// DEBT: These need to live in 'internal/scheduler.h'
struct MutexContext
{
    const bool use_mutex_ : 1;
    const bool in_isr_ : 1;

    constexpr bool use_mutex() const { return use_mutex_; }
    constexpr bool in_isr() const { return in_isr_; }

    constexpr MutexContext(bool in_isr, bool use_mutex = true) :
        use_mutex_(use_mutex),
        in_isr_(in_isr)
    {

    }
};

typedef MutexContext SchedulerContextFlags;

template <class TScheduler>
struct SchedulerContextBase : SchedulerContextFlags,
    TScheduler::impl_type::context_type
{
    typedef TScheduler scheduler_type;
    typedef typename TScheduler::impl_type::context_type impl_context_type;

    scheduler_type& scheduler_;

    constexpr const scheduler_type& scheduler() const { return scheduler_; }

    inline typename scheduler_type::mutex_provider::evaporated_type mutex()
    {
        return scheduler_.mutex();
    }

    constexpr SchedulerContextBase(scheduler_type& s, bool in_isr, bool use_mutex) :
        SchedulerContextFlags(in_isr, use_mutex),
        scheduler_(s) {}

protected:
    // DEBT: Sloppy
    bool flag1_ : 1;
    bool flag2_ : 1;
    bool flag3_ : 1;

public:
    bool is_processing() const { return flag1_; }
    void is_processing(bool v) { flag1_ = v; }
};

template <class TScheduler, class TUserContext = estd::monostate>
struct SchedulerContext :
    SchedulerContextBase<TScheduler>
{
    typedef SchedulerContextBase<TScheduler> base_type;
    typedef typename base_type::scheduler_type scheduler_type;
    typedef TUserContext user_context_type;

    // DEBT: Do the whole value/reference evaporator routine here
    user_context_type user_context_;

    user_context_type& user_context() { return user_context_; }

    constexpr SchedulerContext(scheduler_type& s, user_context_type& uc, bool in_isr, bool use_mutex = true) :
        base_type(s,in_isr, use_mutex),
        user_context_(uc)
    {

    }

    constexpr SchedulerContext(scheduler_type& s, bool in_isr, bool use_mutex = true) :
        base_type(s, in_isr, use_mutex)
    {

    }
};


struct noop_mutex
{
    static void lock(SchedulerContextFlags) {}

    static void unlock(SchedulerContextFlags) {}
};

namespace scheduler { namespace impl {

// For full scheduler context, not just scheduler impl context
struct ReferenceContextFactory
{
    template <class TScheduler, class TUserContext>
    inline static SchedulerContext<TScheduler, TUserContext> create_context(
        TScheduler& scheduler, TUserContext& user_context, bool in_isr, bool use_mutex = true)
    {
        return SchedulerContext<TScheduler, TUserContext>(scheduler, user_context, in_isr, use_mutex);
    }
};

struct ReferenceBaseBase
{
    typedef internal::noop_mutex mutex;

#if __cpp_variadic_templates
    // EXPERIMENTAL
    template <class F, typename ...TArgs>
    void project_emplace(F&& f, TArgs&&...args)
    {
        f(std::forward<TArgs>(args)...);
    }
#endif

    // DEBT: Improve naming - context_type is just for schedule impl context,
    // while context_factory is for schedule context
    typedef estd::monostate context_type;
    typedef ReferenceContextFactory context_factory;

    // Simpler version of subject/observer mechanism, tuned for impl-specific usage
    template <class T, typename TTimePoint, class TContext>
    inline void on_processing(T& value, TTimePoint, TContext& context) {}

    template <class T, typename TTimePoint, class TContext>
    inline void on_processed(T* value, TTimePoint, TContext& context) {}

    template <class T, class TContext>
    inline void on_scheduling(T& value, TContext& context) {}

    template <class T, class TContext>
    inline void on_scheduled(const T& value, TContext& context) {}

    template <class TScheduler>
    inline void start(TScheduler*) {}

    inline void stop() {}

protected:
    // EXPERIMENTAL accessor class
    template <class TScheduler>
    struct Buddy
    {
        typedef TScheduler scheduler_type;
        typedef typename scheduler_type::container_type container_type;

        static container_type& container(scheduler_type& s)
        {
            return s.event_queue.container();
        }

        static int container_sz(scheduler_type& s)
        {
            return s.event_queue.container().size();
        }
    };
};

// Deduces int_type and duration from more complicated TTimePoints
template <typename TTimePoint>
struct TimePointTraits;

template <typename TTimePoint = void, class TTimePointTraits = TimePointTraits<TTimePoint> >
struct ReferenceBase;

struct ReferenceBaseTag {};



template <typename TInt>
struct TimePointTraits
{
    typedef TInt time_point;    ///< Core time_point on which scheduling is based
    typedef TInt int_type;      ///< Underlying int type - useful for complex time_points like chrono's
    typedef TInt duration;      ///< Type to do additions, subtractions, etc. on time_point - mimic/aliases chrono

    // EXPERIMENTAL
    static constexpr bool is_chrono() { return false; }

    static constexpr int_type duration_zero() { return 0; }
    static constexpr int_type time_point_max() { return estd::numeric_limits<int_type>::max(); }
};


template <typename Rep, typename Period>
struct TimePointTraits<estd::chrono::duration<Rep, Period> >
{
    typedef estd::chrono::duration<Rep, Period> duration;
    typedef duration time_point;
    typedef Rep int_type;

    static constexpr bool is_chrono() { return true; }

    static constexpr duration duration_zero() { return duration::zero(); }
    static constexpr time_point time_point_max() { return time_point::max(); }
};


template <typename TClock, typename TDuration>
struct TimePointTraits<estd::chrono::time_point<TClock, TDuration> >
{
    typedef TDuration duration;
    typedef estd::chrono::time_point<TClock, TDuration> time_point;
    typedef typename TDuration::rep int_type;

    static constexpr bool is_chrono() { return true; }

    static constexpr duration duration_zero() { return duration::zero(); }
    static constexpr time_point time_point_max() { return time_point::max(); }
};


template <typename TTimePoint, class TTimePointTraits>
struct ReferenceBase : ReferenceBaseBase, ReferenceBaseTag
{
    typedef TTimePointTraits time_point_traits;
    typedef typename time_point_traits::time_point time_point;
    typedef typename time_point_traits::duration duration;

    // Reference implementation only.  Yours may be quite different
    struct value_type
    {
        typedef typename time_point_traits::time_point time_point;

        time_point event_due_;

        const time_point& event_due() const { return event_due_; }

        // DEBT: c++11 dependent
        value_type() = default;     // DEBT: priority_queue may need this, but see if we can phase that out.  If not, document why
        value_type(const value_type& copy_from) = default;

        ESTD_CPP_CONSTEXPR_RET value_type(time_point event_due) : event_due_(event_due) {}

        bool process(time_point) { return false; }
        inline void rebase(duration v) { event_due_ -= v; }
    };
};

template <class T, class TTimePoint = decltype(T().event_due()), class Enabled = void>
struct Reference;

/// Reference base implementation for scheduler impl
/// \tparam T consider this the system + app data structure
template <class T, class TTimePoint>
struct Reference<T, TTimePoint, typename estd::enable_if<!estd::is_pointer<T>::value>::type> : ReferenceBase<TTimePoint>
{
    typedef ReferenceBase<TTimePoint> base_type;
    typedef T value_type;
    typedef T item_type;

    typedef typename base_type::time_point time_point;
    static bool ESTD_CPP_CONSTEXPR_RET is_pointer() { return false; }

    /// Retrieve specialized wake up time from T
    /// \param value
    /// \return
    static time_point get_time_point(const T& value)
    {
        return value.event_due();
    }

    ///
    /// \param value
    /// \param current_time actual current time
    /// \return true = reschedule requested, false = one shot
    static bool process(T& value, time_point current_time)
    {
        return value.process(current_time);
    }
};


template <class T, class TTimePoint>
struct Reference<T, TTimePoint, typename estd::enable_if<estd::is_pointer<T>::value>::type> : ReferenceBase<TTimePoint>
{
    typedef ReferenceBase<TTimePoint> base_type;
    typedef typename std::remove_pointer<T>::type item_type;
    typedef T value_type;

    typedef typename base_type::time_point time_point;
    static bool ESTD_CPP_CONSTEXPR_RET is_pointer() { return true; }

    /// Retrieve specialized wake up time from T
    /// \param value
    /// \return
    static time_point get_time_point(const T value)
    {
        return value->event_due();
    }

    ///
    /// \param value
    /// \param current_time actual current time
    /// \return true = reschedule requested, false = one shot
    static bool process(T value, time_point current_time)
    {
        return value->process(current_time);
    }
};


struct TraditionalBase : ReferenceBase<unsigned long>
{
    typedef ReferenceBase<unsigned long> base_type;
    typedef typename base_type::time_point time_point;

    struct control_structure
    {
        typedef bool (*handler_type)(control_structure* c, time_point current_time);

        time_point wake_time;
        handler_type handler;

        void* data;
    };
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
struct Function : ReferenceBase<TTimePoint>
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
};

}}

}}
