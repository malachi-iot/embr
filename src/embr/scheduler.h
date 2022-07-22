//
// Created by malachi on 4/6/22.
//
#pragma once

#include <estd/queue.h>
#include <estd/chrono.h>

#include "observer.h"

namespace embr {

namespace internal {

// TODO: Move this out to estd
template <class TLockable, bool const_lock = false>
class scoped_lock;

template <class TLockable>
class scoped_lock<TLockable, false>
{
    TLockable lockable;

public:
    typedef typename TLockable::value_type value_type;

private:
    value_type& value;

public:
#ifdef FEATURE_CPP_MOVESEMANTIC
    scoped_lock(TLockable&& lockable) :
        lockable(std::move(lockable)),
        value(lockable.lock())
    {
    }
#endif

    scoped_lock(TLockable& lockable) :
        lockable(lockable),
        value(lockable.lock())
    {
    }

    ~scoped_lock()
    {
        lockable.unlock();
    }

    value_type& operator*() { return value; }
};

/// Reference scheduler item traits
/// \tparam T consider this the system + app data structure
template <class T>
struct SchedulerImpl
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
};

namespace events {

template<class TSchedulerImpl>
struct Scheduler
{
    typedef TSchedulerImpl traits_type;
};


template<class TSchedulerImpl, bool is_const = true>
struct ValueBase : Scheduler<TSchedulerImpl>
{
    typedef typename TSchedulerImpl::value_type _value_type;
    typedef typename estd::conditional<is_const, const _value_type, _value_type>::type value_type;

    // DEBT: Needs better name, this represents the control/meta structure going in
    // the sorted heap.  Be advised the heap one may be a copy of this
    value_type& value;

    ValueBase(value_type& value) : value(value) {}
};

template <class TSchedulerImpl>
struct Scheduling : ValueBase<TSchedulerImpl>
{
    typedef ValueBase<TSchedulerImpl> base_type;

    Scheduling(typename base_type::value_type& value) : base_type(value) {}
};


// DEBT: Consider how to semi standardize collection operation events, somewhat similar to how C#
// does with IObservableCollection
template <class TSchedulerImpl>
struct Scheduled : ValueBase<TSchedulerImpl>
{
    typedef ValueBase<TSchedulerImpl> base_type;

    Scheduled(typename base_type::value_type& value) : base_type(value) {}
};


template <class TSchedulerImpl>
struct Removed : ValueBase<TSchedulerImpl>
{
    typedef ValueBase<TSchedulerImpl> base_type;

    Removed(typename base_type::value_type& value) : base_type(value) {}
};


template <class TSchedulerImpl>
struct Processing : ValueBase<TSchedulerImpl>
{
    typedef ValueBase<TSchedulerImpl> base_type;
    typedef typename base_type::traits_type::time_point time_point;

    const time_point current_time;

    Processing(typename base_type::value_type& value, time_point current_time) :
        base_type(value), current_time(current_time)
    {}
};

}

/**
 *
 * @tparam TContainer raw container for priority_queue usage.  Its value_type must be
 * copyable as indeed priority_queue stores each of these entries by value
 * @tparam TImpl
 * @tparam TSubject optional observer which can listen for schedule and remove events
 */
template <class TContainer,
    class TImpl = SchedulerImpl<typename TContainer::value_type>,
    class TSubject = embr::void_subject
    >
class Scheduler :
    protected estd::internal::struct_evaporator<TSubject>,
    protected estd::internal::struct_evaporator<TImpl>
{
protected:
    typedef TContainer container_type;

public:
    typedef typename container_type::value_type value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef TImpl impl_type;
    typedef typename impl_type::time_point time_point;

protected:
    typedef estd::internal::struct_evaporator<TSubject> subject_provider;
    typedef estd::internal::struct_evaporator<TImpl> impl_provider;

    typedef events::Scheduling<impl_type> scheduling_event_type;
    typedef events::Scheduled<impl_type> scheduled_event_type;
    typedef events::Removed<impl_type> removed_event_type;
    typedef events::Processing<impl_type> processing_event_type;

    inline typename subject_provider::evaporated_type subject()
    {
        return subject_provider::value();
    }

public:
    inline typename impl_provider::evaporated_type impl()
    {
        return impl_provider::value();
    }

protected:
    // NOTE: This particular comparer demands that get_time_point be a static
    struct Comparer
    {
        bool operator ()(const_reference left, const_reference right)
        {
            time_point l_tp = impl_type::get_time_point(left);
            time_point r_tp = impl_type::get_time_point(right);

            return l_tp > r_tp;
        }
    };

    typedef estd::priority_queue<value_type, container_type, Comparer> priority_queue_type;
    typedef typename priority_queue_type::accessor accessor;
    typedef typename priority_queue_type::size_type size_type;

    priority_queue_type event_queue;

    void do_notify_scheduling(value_type& value)
    {
        subject_provider::value().notify(scheduling_event_type(value), *this);
    }

    bool process_impl(value_type& t, time_point current_time, estd::monostate)
    {
        return impl().process(t, current_time);
    }

    template <class TContext>
    inline bool process_impl(value_type& t, time_point current_time, TContext& context)
    {
        return impl().process(t, current_time, context);
    }



public:
    time_point top_time() const
    {
        // DEBT: Use impl() here once we have estd sorted to give us a
        // const_evaporator in all conditions
        time_point t = impl_type::get_time_point(event_queue.top().lock());
        event_queue.top().unlock();
        return t;
    }

public:
    Scheduler() = default;
    //Scheduler(TSubject subject) : subject_provider(subject) {}
    Scheduler(const TSubject& subject) : subject_provider(subject) {}
#ifdef FEATURE_CPP_MOVESEMANTIC
    Scheduler(TSubject&& subject) : subject_provider(std::move(subject))
    {

    }
#endif


    void schedule(const value_type& value)
    {
        do_notify_scheduling(value);

        event_queue.push(value);

        subject_provider::value().notify(scheduled_event_type());
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    void schedule(value_type&& value)
    {
        do_notify_scheduling(value);

        event_queue.push(std::move(value));

        subject_provider::value().notify(scheduled_event_type(value));
    }
#endif

#ifdef FEATURE_CPP_VARIADIC
    template <class ...TArgs>
    void schedule(TArgs&&...args)
    {
        accessor value = event_queue.emplace_with_notify(
            [&](const value_type& v)
            {
                // announce emplaced item before we actually sort it
                subject_provider::value().notify(scheduling_event_type(v), *this);
            },
            std::forward<TArgs>(args)...);

        subject_provider::value().notify(scheduled_event_type(value.lock()));

        value.unlock();
    }


    /// Presumes that TImpl::value_type's first parameter is wakeup time
    /// \tparam TArgs
    /// \param args
    template <class ...TArgs>
    void schedule_now(TArgs&&...args)
    {
        time_point now = impl().now();
        schedule(now, std::forward<TArgs>(args)...);
    }
#endif

    size_type size() const
    {
        return event_queue.size();
    }

    accessor top() const
    {
        return event_queue.top();
    }

    /*
    void pop()
    {
        scoped_lock<accessor> t(top());

        event_queue.pop();

        subject_provider::value().notify(removed_event_type (*t));
    } */

    template <class TContext>
    void process(time_point current_time, TContext& context)
    {
        while(!event_queue.empty())
        {
            scoped_lock<accessor> t(top());
            time_point eval_time = impl().get_time_point(*t);

            if(current_time >= eval_time)
            {
                subject_provider::value().notify(processing_event_type (*t, current_time), context);

                bool reschedule_requested = process_impl(*t, current_time, context);

                // Need to do this because *t is a pointer into event_queue and the pop moves
                // items around.
                value_type copied = *t;

                event_queue.pop();

                subject_provider::value().notify(removed_event_type (copied), context);

                if(reschedule_requested)
                {
                    do_notify_scheduling(copied);

                    // DEBT: Doesn't handle move variant
                    event_queue.push(copied);

                    subject_provider::value().notify(scheduled_event_type (copied), context);
                }
            }
            else
                return;
        }
    }


    /// Processes current_time as 'now' as reported by traits
    template <class TContext, class TEnable =
        typename estd::enable_if<
            !estd::is_same<TContext, time_point>::value>::type >
    void process(TContext& context)
    {
        time_point current_time = impl().now();
        process(current_time);
    }


    void process(time_point current_time)
    {
        estd::monostate context;

        process(current_time, context);
    }

    void process()
    {
        estd::monostate context;

        process(context);
    }

    template <class T>
    value_type* match(const T& value)
    {
        // brute force through underlying container attempting to match

        for(auto& i : event_queue.container())
        {
            if(i.match(value))
                return &i;
        }

        return nullptr;
    }
};

namespace experimental {

// DEBT: Rename to FunctorImpl and put under scheduler namespace
template <typename TTimePoint>
struct FunctorTraits
{
    typedef TTimePoint time_point;
    typedef estd::experimental::function_base<void(time_point*, time_point)> function_type;

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
            func(func)
        {}

        // Don't give anybody false hope that we're gonna housekeep function_type
        control_structure(time_point wake, function_type&& func) = delete;

        // DEBT: See Item2Traits
        control_structure() = default;

        bool match(const function_type& f)
        {
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

}

// DEBT: Putting this here under internal to reflect we're still fleshing things out
namespace layer1 {

template <class TTraits, int count, class TSubject = void_subject>
struct Scheduler :
    internal::Scheduler<estd::layer1::vector<typename TTraits::value_type, count>, TTraits, TSubject>
{
    typedef internal::Scheduler<estd::layer1::vector<typename TTraits::value_type, count>, TTraits, TSubject> base_type;

    ESTD_CPP_FORWARDING_CTOR(Scheduler)
};

}

}


}