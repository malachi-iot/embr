//
// Created by malachi on 4/6/22.
//
#pragma once

#include <estd/queue.h>
#include <estd/chrono.h>

#include "observer.h"
#include "internal/scheduler.h"

namespace embr {

namespace internal {

namespace scheduler {

// DEBT: See Scheduler constructor comments
struct impl_params_tag {};

}


namespace events {

template<class TSchedulerImpl>
struct Scheduler
{
    typedef TSchedulerImpl impl_type;
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
    typedef typename base_type::impl_type::time_point time_point;

    const time_point current_time;

    Processing(typename base_type::value_type& value, time_point current_time) :
        base_type(value), current_time(current_time)
    {}
};


template <class TSchedulerImpl>
struct Processed : Scheduler<TSchedulerImpl>
{
    typedef Scheduler<TSchedulerImpl> base_type;
    typedef typename base_type::impl_type::time_point time_point;

    const time_point current_time;

    // DEBT: Need to actually store 'value'

    Processed(typename base_type::impl_type::value_type* value, time_point current_time) :
        current_time(current_time)
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
    class TImpl,
    class TSubject = embr::void_subject
    >
class Scheduler :
    public TImpl,
    protected estd::internal::struct_evaporator<TSubject>,
    protected estd::internal::struct_evaporator<typename TImpl::mutex>
{
    typedef Scheduler this_type;

    // EXPERIMENTAL - this_type maybe TOO specific?
    //friend class embr::internal::scheduler::impl::ReferenceBaseBase::Buddy<this_type>;

    // EXPERIMENTAL - this flavor works, thanks to
    // https://stackoverflow.com/questions/3292795/how-to-declare-a-templated-struct-class-as-a-friend
    template <typename>
    friend class embr::internal::scheduler::impl::ReferenceBaseBase::Buddy;

protected:
    typedef TContainer container_type;

public:
    typedef typename container_type::value_type value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef TImpl impl_type;
    typedef typename impl_type::context_factory context_factory;
    typedef typename impl_type::time_point time_point;
    typedef SchedulerContextBase<this_type> context_base_type;

    // DEBT: May want c++03 guards here
    template <class TUserContext = estd::monostate>
    using context_type = SchedulerContext<this_type, TUserContext>;

    // EXPERIMENTAL, different way to get at typedefs
    struct event
    {
        typedef events::Scheduling<impl_type> scheduling;
    };

    typedef typename impl_type::mutex mutex_type;
    typedef estd::internal::struct_evaporator<mutex_type> mutex_provider;

protected:
    typedef estd::internal::struct_evaporator<TSubject> subject_provider;

    typedef events::Scheduling<impl_type> scheduling_event_type;
    typedef events::Scheduled<impl_type> scheduled_event_type;
    typedef events::Removed<impl_type> removed_event_type;
    typedef events::Processing<impl_type> processing_event_type;

    inline typename subject_provider::evaporated_type subject()
    {
        return subject_provider::value();
    }

public:
    inline impl_type& impl()
    {
        return *this;
    }

    inline typename mutex_provider::evaporated_type mutex()
    {
        return mutex_provider::value();
    }

    // Creates a default context if non is specifically provided
    // DEBT: Some kind of policy should feed the parameters to default context
    inline context_type<> create_context(bool in_isr = false, bool use_mutex = true)
    {
        estd::monostate uc;

        return context_factory::create_context(*this, uc, in_isr, use_mutex);
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

    template <class TContext>
    inline void do_notify_processing(value_type& value, time_point current_time, context_type<TContext>& context)
    {
        impl_type::on_processing(value, current_time, context);
        subject_provider::value().notify(processing_event_type(value, current_time), context);
    }

    template <class TContext>
    inline void do_notify_processed(value_type* value, time_point current_time, context_type<TContext>& context)
    {
        impl_type::on_processed(value, current_time, context);
        subject_provider::value().notify(events::Processed<impl_type>(value, current_time), context);
    }

    template <class TContext>
    inline void do_notify_scheduling(value_type& value, context_type<TContext>& context)
    {
        impl().on_scheduling(value, context);
        subject_provider::value().notify(scheduling_event_type(value), context);
    }

    /*
    template <class TContext>
    void do_notify_scheduling(const value_type& value, context_type<TContext>& context)
    {
        impl().on_scheduling(value, context);
        subject_provider::value().notify(scheduling_event_type(value), context);
    } */

    inline void do_notify_scheduling(value_type& value)
    {
        context_type<> context = create_context();
        do_notify_scheduling(value, context);
    }

    // Doing const because this may be the passed-in vs. copied/emplaced one
    // though we expect content to be identical
    template <class TContext>
    inline void do_notify_scheduled(const value_type& value, context_type<TContext>& context)
    {
        impl().on_scheduled(value, context);

        subject_provider::value().notify(scheduled_event_type(value), context);
    }

    inline void do_notify_scheduled(const value_type& value)
    {
        context_type<> context = create_context();
        do_notify_scheduled(value, context);
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

    template <class TContext>
    inline bool process_impl(value_type& t, time_point current_time, context_type<TContext>& context)
    {
        // DEBT: We actually would prefer full SchedulerContext to be passed if we can
        return process_impl(t, current_time, context.user_context());
    }

    // DEBT: Also mixes concerns and checks for if we should even do mutex
    struct mutex_guard
    {
        context_base_type& context;

    private:
        inline void lock()
        {
            if(context.use_mutex()) context.mutex().lock(context);
        }

        inline void unlock()
        {
            if(context.use_mutex()) context.mutex().unlock(context);
        }

    public:
        mutex_guard(context_base_type& context) : context(context)
        {
            lock();
        }

        ~mutex_guard() { unlock(); }
    };

    // No mutex is what makes this an internal call
    template <class TContext>
    inline void schedule_internal(value_type& value, context_type<TContext>& context)
    {
        do_notify_scheduling(value, context);

        event_queue.push(value);

        do_notify_scheduled(value, context);
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
#ifdef __cpp_variadic_templates
    // impl-initializing version
    // DEBT: impl_params_tag necessary to disambiguate from TSubject initializers above
    template <typename ...TArgs>
    constexpr Scheduler(scheduler::impl_params_tag, TArgs&&...args) : impl_type(std::forward<TArgs>(args)...)
    {

    }
#endif

    // DEBT: No mutex operation here, be careful!
    void schedule(const value_type& value)
    {
        do_notify_scheduling(value);

        event_queue.push(value);

        do_notify_scheduled(value);
    }

    template <class TContext>
    void schedule(const value_type& value, context_type<TContext>& context)
    {
        mutex_guard m(context);

        schedule_internal(value, context);
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    void schedule(value_type&& value)
    {
        do_notify_scheduling(value);

        event_queue.push(std::move(value));

        do_notify_scheduled(value);
    }
#endif

#ifdef __cpp_variadic_templates
    // DEBT: Not so elegant
    template <class TContext, class ...TArgs>
    void schedule_with_context(internal::SchedulerContext<this_type, TContext>& context, TArgs&&...args)
    {
        mutex_guard m(context);

        accessor value = event_queue.emplace_with_notify(
            [&](value_type& v)
            {
                // announce emplaced item before we actually sort it
                do_notify_scheduling(v, context);
            },
            std::forward<TArgs>(args)...);

        do_notify_scheduled(value, context);

        value.unlock();
    }

    template <class ...TArgs>
    inline void schedule(TArgs&&...args)
    {
        context_type<> context = create_context();
        schedule_with_context(context, std::forward<TArgs>(args)...);
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

    bool empty() const
    {
        return event_queue.empty();
    }


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
    void process(time_point current_time, context_type<TContext>& context);


    /// Processes current_time as 'now' as reported by traits
    template <class TContext, class TEnable =
        typename estd::enable_if<
            !estd::is_same<TContext, time_point>::value>::type >
    void process(TContext& context)
    {
        time_point current_time = impl().now();
        process(current_time, context);
    }


    void process(time_point current_time)
    {
        context_type<> context = create_context();

        process(current_time, context);
    }

    void process_in_isr(time_point current_time)
    {
        context_type<> context = create_context(true);

        process(current_time, context);
    }

    void process()
    {
        context_type<> context = create_context();

        process(context);
    }

    template <class T>
    value_type* match(const T& value)
    {
        context_type<> context = create_context();
        mutex_guard m(context);

        // brute force through underlying container attempting to match

        for(auto& i : event_queue.container())
        {
            if(i.match(value))
                return &i;
        }

        return nullptr;
    }

    void unschedule(reference v)
    {
        context_type<> context = create_context();
        mutex_guard m(context);

        event_queue.erase(v);
    }

#ifdef __cpp_variadic_templates
    // EXPERIMENTAL but liking this - "self hosted" schedulers
    template <class ...TArgs>
    inline void start(TArgs&&...args)
    {
        impl_type::start(this, std::forward<TArgs>(args)...);
    }

    inline void stop()
    {
        impl_type::stop();
    }
#endif
};


// DEBT: Putting this here under internal to reflect we're still fleshing things out
namespace layer1 {

template <int count,
    class TImpl = embr::internal::scheduler::impl::Function<estd::chrono::steady_clock::time_point>,
    class TSubject = void_subject>
struct Scheduler :
    embr::internal::Scheduler<estd::layer1::vector<typename TImpl::value_type, count>, TImpl, TSubject>
{
    typedef embr::internal::Scheduler<estd::layer1::vector<typename TImpl::value_type, count>, TImpl, TSubject> base_type;

    ESTD_CPP_FORWARDING_CTOR(Scheduler)
};

}

}


}
