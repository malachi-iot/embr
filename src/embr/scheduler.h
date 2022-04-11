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
/// \tparam T
template <class T>
struct schedule_item_traits
{
    typedef T value_type;

    typedef estd::chrono::steady_clock::time_point time_point;

    static time_point get_time_point(const T& value)
    {
        return value.event_due;
    }

    ///
    /// \param value
    /// \return true = reschedule requested, false = one shot
    static bool process(T& value)
    {
        return false;
    }
};

namespace events {

template<class TTraits>
struct Scheduler
{

};


template<class TTraits>
struct ValueBase : Scheduler<TTraits>
{
    typedef typename TTraits::value_type value_type;

    // DEBT: Needs better name, this represents the control/meta structure going in
    // the sorted heap.  Be advised the heap one may be a copy of this
    value_type& value;

    ValueBase(value_type& value) : value(value) {}
};

template<class TTraits>
struct Scheduled : ValueBase<TTraits>
{
    typedef ValueBase<TTraits> base_type;

    Scheduled(typename base_type::value_type& value) : base_type(value) {}
};


template<class TTraits>
struct Removed : ValueBase<TTraits>
{
    typedef ValueBase<TTraits> base_type;

    Removed(typename base_type::value_type& value) : base_type(value) {}
};

}

/**
 *
 * @tparam TContainer raw container for priority_queue usage.  Its value_type must be
 * copyable as indeed priority_queue stores each of these entries by value
 * @tparam TTraits
 * @tparam TSubject optional observer which can listen for schedule and remove events
 */
template <class TContainer,
    class TTraits = schedule_item_traits<typename TContainer::value_type>,
    class TSubject = embr::void_subject
    >
class Scheduler : protected estd::internal::struct_evaporator<TSubject>
{
    typedef TContainer container_type;
    typedef typename container_type::value_type value_type;
    typedef value_type& reference;

    typedef TTraits traits_type;
    typedef typename traits_type::time_point time_point;

    typedef estd::internal::struct_evaporator<TSubject> subject_provider;
    typedef events::Scheduled<traits_type> scheduled_event_type;
    typedef events::Removed<traits_type> removed_event_type;

    struct Comparer
    {
        bool operator ()(const reference left, const reference right)
        {
            time_point l_tp = traits_type::get_time_point(left);
            time_point r_tp = traits_type::get_time_point(right);

            return l_tp > r_tp;
        }
    };

    typedef estd::priority_queue<value_type, container_type, Comparer> priority_queue_type;
    typedef typename priority_queue_type::accessor accessor;
    typedef typename priority_queue_type::size_type size_type;

    priority_queue_type event_queue;

    time_point top_time() const
    {
        time_point t = traits_type::get_time_point(event_queue.top().lock());
        event_queue.top().unlock();
        return t;
    }

public:
    void schedule(const value_type& value)
    {
        event_queue.push(value);

        subject_provider::value().notify(scheduled_event_type());
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    void schedule(value_type&& value)
    {
        event_queue.push(std::move(value));

        subject_provider::value().notify(scheduled_event_type(value));
    }
#endif

#ifdef FEATURE_CPP_VARIADIC
    template <class ...TArgs>
    void schedule(TArgs&&...args)
    {
        accessor value = event_queue.emplace(std::forward<TArgs>(args)...);

        subject_provider::value().notify(scheduled_event_type(value.lock()));

        value.unlock();
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

    void process(time_point current_time)
    {
        while(!event_queue.empty())
        {
            scoped_lock<accessor> t(top());
            time_point eval_time = traits_type::get_time_point(*t);

            if(current_time >= eval_time)
            {
                bool reschedule_requested = traits_type::process(*t);

                event_queue.pop();

                subject_provider::value().notify(removed_event_type (*t));

                if(reschedule_requested)
                {
                    // FIX: Doesn't handle move variant
                    // FIX: May be badly behaved since 'pop' above may deallocate some things
                    event_queue.push(*t);

                    subject_provider::value().notify(scheduled_event_type (*t));
                }
            }
            else
                return;
        }
    }
};

}


}