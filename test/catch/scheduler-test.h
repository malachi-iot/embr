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

    virtual bool process(time_point current_time)
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

    virtual bool process(time_point current_time)
    {
        ++counter;
        //t += std::chrono::seconds(5);
        return false;
    }
};




}}