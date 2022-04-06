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

template <class TContainer>
class Scheduler
{
    typedef TContainer container_type;
    typedef typename container_type::value_type value_type;
    typedef value_type& reference;

    typedef schedule_item_traits<value_type> traits_type;
    typedef typename traits_type::time_point time_point;

    static bool compare(const reference left, const reference right)
    {
        time_point l_tp = traits_type::get_time_point(left);
        time_point r_tp = traits_type::get_time_point(right);

        return estd::less<time_point>(l_tp, r_tp);
    }

    estd::priority_queue<value_type, container_type, decltype(&compare)> event_queue;

public:
    void schedule(const value_type& value)
    {
        event_queue.push(value);
    }
};

}


}