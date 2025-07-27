#pragma once

#include <estd/queue.h>

#if __cpp_lib_concepts
#include <concepts>
#endif

// DEBT: Lousy name on purpose.  Don't want to call 'scheduler' since that already is used.
// This class identifies from a batch of next-aware children (possibly state machines)
// who the next one up is.  Behavior overlaps with retry v4 behavior as well.
// Big overlap with existing scheduler to the point they probably could be combined
// AKA "scheduler jr"

namespace embr { namespace experimental {

#if __cpp_lib_concepts
namespace concepts {

template <class T>
concept NexterItem = requires(T)
{
    T::time_point;
};

template <class T>
concept NexterTraits = requires
{
    typename T::value_type;
    typename T::const_reference;
    typename T::time_point;
    typename T::compare;
    //typename T::time_point T::next(std::declval<typename T::value_type>());
};

}
#endif

// DEBT: Seems like time_point really ought to consolidate here too
template <class Item>
struct nexter_item_traits
{
    using time_point = typename Item::time_point;

    ESTD_CPP_STD_VALUE_TYPE(Item)

    struct compare
    {
        constexpr bool operator()(const_pointer lhs, const_pointer rhs) const
        {
            return lhs->next() > rhs->next();
        }
    };

    static constexpr time_point next(const_reference v) { return v.next(); }
};

template <ESTD_CPP_CONCEPT(concepts::NexterTraits) Traits>
class nexter
{
    using traits = Traits;

    ESTD_CPP_STD_VALUE_TYPE(typename traits::value_type)

    estd::layer1::priority_queue<pointer, 10, typename traits::compare> items_;

public:
    using time_point = typename traits::time_point;

    ///
    /// @param now
    /// @return true if item was processed and rescheduled, false otherwise
    bool process_one(time_point now);
    void process(time_point now);
    bool reschedule(pointer);

    constexpr bool empty() const { return items_.empty(); }

    constexpr const_reference top() const { return *items_.top(); }

    constexpr time_point next() const
    {
        return traits::next(*items_.top());
    }

    // NOTE: More echoes and overlap with scheduler
    constexpr bool ready(time_point now) const
    {
        return items_.empty() ? false : now >= next();
    }
};


template <ESTD_CPP_CONCEPT(concepts::NexterTraits) Traits>
bool nexter<Traits>::process_one(time_point now)
{
    if(items_.empty())  return false;

    pointer t = items_.top();
    const time_point next = traits::next(*t);

    // Are we actually at a next up situation?
    if(now >= next)
    {
        items_.pop();
        t->process();

        // Now that we've processed, next may have changed.  If we have something
        // to reschedule, do so
        // NOTE: This is so far the only signal a reschedule is desired, a differing next.
        if(traits::next(*t) != next)
        {
            items_.push(t);
            return true;
        }
    }

    return false;
}

template <ESTD_CPP_CONCEPT(concepts::NexterTraits) Traits>
void nexter<Traits>::process(time_point now)
{
    process_one(now);
}

template <ESTD_CPP_CONCEPT(concepts::NexterTraits) Traits>
bool nexter<Traits>::reschedule(pointer v)
{
    bool erased = items_.erase_if([v](pointer item) { return v == item; });
    items_.push(v);
    return erased;
}

template <class Item, ESTD_CPP_CONCEPT(concepts::NexterTraits) Traits = nexter_item_traits<Item>>
using nexter2 = nexter<Traits>;


template <class TimePoint>
class ref_nexter
{
    int id_{};
    TimePoint next_{};
    bool toggle_{};

public:
    constexpr explicit ref_nexter(int id) : id_(id) {}

    constexpr int id() const { return id_; }
    constexpr bool toggle() const { return toggle_; }

    using time_point = TimePoint;

    constexpr const time_point& next() const { return next_; }
    void next(time_point v) { next_ = v; }

    // Feature of nexter - this is only called when 'now' >= next
    void process()
    {
        next_ += 4;
        toggle_ = !toggle_;
    }
};

class ref_adapter
{
public:
    int counter_{};

    template <class TimePoint>
    void process(const ref_nexter<TimePoint>& appointer)
    {
        if(appointer.toggle()) counter_++;
    }
};

template <class Appointer, class Adapter>
class nexter_processor :
    public Appointer,
    public Adapter
{
public:
    // DEBT: What about you, Adapter?
    template <class ...Args>
    constexpr nexter_processor(Args&&...args) : Appointer(std::forward<Args>(args)...) {}

    void process()
    {
        Adapter::process(*this);
        Appointer::process();
    }
};


}}