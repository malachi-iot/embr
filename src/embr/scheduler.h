//
// Created by malachi on 4/6/22.
//
#pragma once

#include <estd/queue.h>
#include <estd/chrono.h>

namespace embr {

namespace internal {

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

template <class T>
struct schedule_item_traits
{
    typedef T value_type;

    typedef estd::chrono::steady_clock::time_point time_point;

    static time_point get_time_point(const T& value)
    {
        return value.event_due;
    }

    static void process(T& value) {}
};

template <class TContainer, class TTraits = schedule_item_traits<typename TContainer::value_type>>
class Scheduler
{
    typedef TContainer container_type;
    typedef typename container_type::value_type value_type;
    typedef value_type& reference;

    typedef TTraits traits_type;
    typedef typename traits_type::time_point time_point;

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
    }

#ifdef FEATURE_CPP_MOVESEMANTIC
    void schedule(value_type&& value)
    {
        event_queue.push(std::move(value));
    }
#endif

#ifdef FEATURE_CPP_VARIADIC
    template <class ...TArgs>
    void schedule(TArgs&&...args)
    {
        event_queue.emplace(std::forward<TArgs>(args)...);
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
        event_queue.top().unlock();
        event_queue.pop();
    } */

    void process(time_point current_time)
    {
        while(!event_queue.empty())
        {
            scoped_lock<accessor> t(top());
            time_point eval_time = traits_type::get_time_point(*t);

            if(current_time >= eval_time)
            {
                traits_type::process(*t);

                event_queue.pop();
            }
            else
                return;
        }
    }
};

}


}