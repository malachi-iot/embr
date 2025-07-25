#pragma once

#include <estd/queue.h>

// DEBT: Lousy name on purpose.  Don't want to call 'scheduler' since that already is used.
// This class identifies from a batch of next-aware children (possibly state machines)
// who the next one up is.  Behavior overlaps with retry v4 behavior as well.
// Big overlap with existing scheduler to the point they probably could be combined
// AKA "scheduler jr"

namespace embr { namespace experimental {

template <class TimePoint, class Item>
class nexter
{
    ESTD_CPP_STD_VALUE_TYPE(Item)

    struct less
    {
        constexpr bool operator()(const_pointer lhs, const_pointer rhs) const
        {
            return lhs->next() < rhs->next();
        }
    };

    estd::layer1::priority_queue<pointer, 10, less> items_;

public:
    using time_point = TimePoint;

    void process_one(time_point now);
    void process(time_point now);
    void reschedule(pointer);

    constexpr bool empty() const { return items_.empty(); }

    constexpr time_point next() const
    {
        return items_.top()->next();
    }
};


template <class TimePoint, class Item>
void nexter<TimePoint, Item>::process_one(time_point now)
{
    if(items_.empty())  return;

    pointer t = items_.top();

    // Are we actually at a next up situation?
    if(now >= t->next())
    {
        items_.pop();
        t->process();

        // Now that we've processed, next may have changed.  If we have something
        // to reschedule, do so
        if(t->next() > now)
        {
            items_.push(t);
        }
    }
}

template <class TimePoint, class Item>
void nexter<TimePoint, Item>::process(time_point now)
{
    process_one(now);
}

template <class TimePoint, class Item>
void nexter<TimePoint, Item>::reschedule(pointer v)
{
    items_.erase_if([v](pointer item) { return v == item; });
    items_.push(v);
}

template <class TimePoint>
class ref_nexter
{
    TimePoint next_{};

public:
    using time_point = TimePoint;

    constexpr const time_point& next() const { return next_; }
    void process()
    {
        next_ += 4;
    }
};


}}