#pragma once

#include <estd/chrono.h>
#include <embr/scheduler.h>

namespace test { namespace scheduler {

struct Item
{
    int event_due_;
    int* counter;

    int event_due() const { return event_due_; }

    Item() = default;

    Item(int event_due, int* counter = nullptr) :
        event_due_{event_due}, counter{counter}
    {}

    bool match(int* c) const { return c == counter; }
};


struct ItemTraits : embr::internal::scheduler::impl::Reference<Item>
{
    typedef embr::internal::scheduler::impl::Reference<Item> base_type;
    typedef int time_point;

    static bool process(Item& item, time_point)
    {
        if(item.counter != nullptr)
            ++(*item.counter);

        return false;
    }

    template <class TScheduler>
    void buddy_test(TScheduler& s)
    {
        typedef typename base_type::Buddy<TScheduler> buddy;
        auto& container = buddy::container(s);
        //int sz = buddy::container_sz(s);
    }
};


struct Item3Traits : embr::internal::scheduler::impl::ReferenceBase<unsigned>
{
    typedef estd::chrono::steady_clock::time_point time_point;

    struct control_structure
    {
        typedef Item3Traits::time_point time_point;

        time_point t;

        virtual bool process(time_point current_time) = 0;
    };

    typedef control_structure* value_type;

    static time_point get_time_point(value_type v) { return v->t; }

    static bool process(value_type v, time_point t)
    {
        return v->process(t);
    }
};



struct Item3ControlStructure1 : Item3Traits::control_structure
{
    int counter = 0;

    bool process(time_point current_time) override
    {
        ++counter;
        // DEBT: Looks like estd::chrono doesn't have these overloads sorted yet
        t += std::chrono::seconds(10);
        return true;
    }
};

struct Item3ControlStructure2 : Item3Traits::control_structure
{
    int counter = 0;

    bool process(time_point current_time) override
    {
        ++counter;
        //t += std::chrono::seconds(5);
        return false;
    }
};


template <class TimePoint, class Duration = TimePoint>
class ref_nexter
{
    int id_{};
    TimePoint next_{};
    TimePoint last_{};
    Duration bump_{};
    bool toggle_{};

public:
    //constexpr explicit ref_nexter(int id) : id_(id) {}
    constexpr explicit ref_nexter(int id, Duration bump) : id_(id), bump_{bump} {}

    constexpr int id() const { return id_; }
    constexpr bool toggle() const { return toggle_; }

    using time_point = TimePoint;

    constexpr const time_point& last() const { return last_; }

    constexpr const time_point& next() const { return next_; }
    void next(time_point v) { next_ = v; }

    // Feature of v2 scheduler - this is only called when 'now' >= next
    void process(time_point now)
    {
        next_ += bump_;
        toggle_ = !toggle_;
        last_ = now;
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
    using time_point = typename Appointer::time_point;

    // DEBT: What about you, Adapter?
    template <class ...Args>
    constexpr nexter_processor(Args&&...args) : Appointer(std::forward<Args>(args)...) {}

    void process(time_point now)
    {
        Adapter::process(*this);
        Appointer::process(now);
    }
};




}}
