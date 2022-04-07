//
// Created by malachi on 4/6/22.
//
#pragma once

#include <estd/queue.h>
#include <estd/chrono.h>

namespace embr {

namespace internal {

template <class T>
struct schedule_item_traits
{
    typedef T value_type;

    typedef estd::chrono::steady_clock::time_point time_point;

    static time_point get_time_point(const T& value)
    {
        return value.event_due;
    }
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
    typedef typename priority_queue_type::accessor  accessor;

    priority_queue_type event_queue;

public:
    void schedule(const value_type& value)
    {
        event_queue.push(value);
    }

    accessor top() const
    {
        return event_queue.top();
    }

    void pop()
    {
        event_queue.top().unlock();
        event_queue.pop();
    }
};

}


}