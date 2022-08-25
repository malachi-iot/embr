#include <catch.hpp>

#include <bitset>

#include <embr/scheduler.h>
#include <embr/observer.h>

// Not available because we test against C++11
//using namespace std::literals::chrono_literals;

struct Item
{
    int event_due;
    int* counter;

    Item() = default;

    Item(int event_due, int* counter = nullptr) :
        event_due{event_due}, counter{counter}
    {}

    bool match(int* c) const { return c == counter; }
};

struct fake_mutex
{
    void lock() {}
    void unlock() {}
};

struct ItemTraits //: embr::internal::SchedulerImpl<int>
{
    typedef Item value_type;
    typedef int time_point;

    static time_point get_time_point(const Item& item) { return item.event_due; }
    static bool process(Item& item, time_point)
    {
        if(item.counter != nullptr)
            ++(*item.counter);

        return false;
    }

    typedef fake_mutex mutex;
};


struct Item2Traits
{
    typedef unsigned time_point;

    // NOTE: It's preferred to keep app state such as counter off in a pointed to struct
    // somewhere, since value_type here is copied around a lot during heap sorts.  That said,
    // it's not any kind of specific violation to keep app data in here
    struct value_type
    {
        int counter = 0;
        time_point wakeup;

        // DEBT: needed because vector doesn't currently know how to do unassigned by itself
        // that said, it is partially  spec behavior of vector:
        // https://stackoverflow.com/questions/29920394/vector-of-class-without-default-constructor
        // however ours deviates because we out of necessity pre allocate
        value_type() = default;

        value_type(time_point wakeup) : wakeup{wakeup} {}
        value_type(const value_type&) = default;

        /* although enabling all the following works, it hits me we prefer to lean heavily towards
         * copy-only because move operations imply greater short term overhead and we want this struct
         * as lightweight as possible
        value_type(value_type&&) = default;

        value_type& operator=(const value_type&) = default;
        value_type& operator=(value_type&&) = default; */
    };

    static time_point get_time_point(const value_type& v) { return v.wakeup; }

    static bool process(value_type& v, time_point)
    {
        v.wakeup += 10;
        ++v.counter;
        return true;
    }

    typedef fake_mutex mutex;
};


struct Item3Traits
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

    typedef fake_mutex mutex;
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



// Legacy c-style handler
bool traditional_handler(
    embr::internal::scheduler::impl::TraditionalBase::control_structure* c,
    unsigned long current_time)
{
    ++(*(int*)c->data);
    return false;
}


using FunctorTraits = embr::internal::scheduler::impl::Function<unsigned>;

struct StatefulFunctorTraits : FunctorTraits
{
    time_point now_ = 0;

    time_point now() { return now_++; }
};


template <class TTraits>
struct SchedulerObserver
{
    typedef TTraits traits_type;

    int added = 0;
    int removed = 0;
    int processed = 0;

    void on_notify(embr::internal::events::Processing<traits_type>)
    {
        ++processed;
    }

    void on_notify(embr::internal::events::Scheduled<traits_type>)
    {
        ++added;
    }

    void on_notify(embr::internal::events::Removed<traits_type>)
    {
        ++removed;
    }
};

TEST_CASE("scheduler test", "[scheduler]")
{
    std::bitset<32> arrived;

    SECTION("impl operations")
    {
        SECTION("copy")
        {
            FunctorTraits::control_structure s1, s2(s1);
            FunctorTraits::control_structure s3, *ps3 = &s3, *ps1 = &s1;

            *ps3 = std::move(*ps1);
        }
    }
    SECTION("one-shot")
    {
        // doesn't have 'accessor', and maybe array isn't a good fit for priority_queue anyway
        //typedef estd::array<int, 4> container_type;
        typedef estd::layer1::vector<Item, 20> container_type;
        embr::internal::Scheduler<container_type, ItemTraits> scheduler;
        int counter = 0;

        scheduler.schedule(Item{5, &counter});
        scheduler.schedule(Item{6, &counter});
        scheduler.schedule(Item{10, &counter});
        scheduler.schedule(Item{11, &counter});

        auto top = scheduler.top();

        const Item& value = top.clock();

        REQUIRE(value.event_due == 5);

        top.cunlock();

        scheduler.process(10);

        REQUIRE(counter == 3);
        REQUIRE(scheduler.size() == 1);
    }
    SECTION("repeating")
    {
        embr::internal::layer1::Scheduler<5, Item2Traits> scheduler;

        scheduler.schedule(5);
        scheduler.schedule(99); // should never reach this one

        for(Item2Traits::time_point i = 0; i < 50; i++)
        {
            scheduler.process(i);
        }

        // being that we continually reschedule, we'll always be at the top
        auto& v = scheduler.top().clock();

        // Should wake up 5 times at 5, 15, 25, 35 and 45
        REQUIRE(v.counter == 5);
    }
    SECTION("events (aux)")
    {
        int counter = 0;
        int _counter = 0;

        typedef estd::layer1::vector<Item, 20> container_type;
        auto o1 = embr::experimental::make_delegate_observer([&counter](
            const embr::internal::events::Scheduled<ItemTraits>& scheduled)
            {
                ++counter;
            });
        auto o2 = embr::experimental::make_delegate_observer([&counter](
            const embr::internal::events::Removed<ItemTraits>& removed)
            {
                --counter;
            });

        // NOTE: May be wanting a ref evaporator not a struct evaporator for scheduler
        auto s = embr::layer1::make_subject(o1, o2);
        typedef decltype(s) subject_type;
        embr::internal::Scheduler<container_type, ItemTraits, subject_type> scheduler(std::move(s));

        scheduler.schedule(Item{5});

        REQUIRE(counter == 1);

        scheduler.schedule(Item{7});

        REQUIRE(counter == 2);

        scheduler.process(6);

        // process removes one, so that will bump down our counter
        REQUIRE(counter == 1);
    }
    SECTION("virtual")
    {
        embr::internal::layer1::Scheduler<5, Item3Traits> scheduler;

        Item3ControlStructure1 schedule1;
        Item3ControlStructure2 schedule2;

        scheduler.schedule(&schedule1);
        scheduler.schedule(&schedule2);

        estd::chrono::steady_clock::time_point now;
        estd::chrono::steady_clock::time_point end = now + std::chrono::seconds(60);

        for(; now < end; now += std::chrono::seconds(2))
        {
            scheduler.process(now);
        }

        REQUIRE(schedule1.counter == 6);
        REQUIRE(schedule2.counter == 1);
    }
    SECTION("traditional")
    {
        SECTION("inline")
        {
            typedef embr::internal::scheduler::impl::Traditional<true> impl_type;
            typedef impl_type::value_type value_type;
            int counter = 0;

            embr::internal::layer1::Scheduler<5, impl_type> scheduler;

            scheduler.schedule(value_type{10, traditional_handler, &counter});
            scheduler.schedule(value_type{20, traditional_handler, &counter});

            scheduler.process(5);
            scheduler.process(10);
            scheduler.process(19);
            scheduler.process(21);

            REQUIRE(counter == 2);
        }
        SECTION("user allocated")
        {
            typedef embr::internal::scheduler::impl::Traditional<false> impl_type;
            //typedef impl_type::value_type value_type;
            int counter = 0;

            embr::internal::layer1::Scheduler<5, impl_type> scheduler;

            impl_type::control_structure
                scheduled1{10, traditional_handler, &counter},
                scheduled2{20, traditional_handler, &counter};

            scheduler.schedule(&scheduled1);
            scheduler.schedule(&scheduled2);

            scheduler.process(5);
            scheduler.process(10);
            scheduler.process(19);
            scheduler.process(21);

            REQUIRE(counter == 2);
        }
    }
    SECTION("experimental")
    {
        // Works well, but overly verbose on account of estd::experimental::function
        // indeed being in progress and experimental
        SECTION("estd::function style")
        {
            embr::internal::layer1::Scheduler<5, FunctorTraits> scheduler;

            SECTION("trivial scheduling")
            {
                auto f_set_only = FunctorTraits::make_function([&arrived](unsigned* wake, unsigned current_time)
                {
                    arrived.set(*wake);
                });

                auto f_set_and_repeat = FunctorTraits::make_function([&arrived](unsigned* wake, unsigned current_time)
                {
                    arrived.set(*wake);
                    *wake += 2;
                });

                scheduler.schedule(11, f_set_and_repeat);
                scheduler.schedule(3, f_set_only);
                scheduler.schedule(9, f_set_only);

                scheduler.process(0);
                scheduler.process(4);
                scheduler.process(9);
                scheduler.process(11);
                scheduler.process(20);

                REQUIRE(!arrived[0]);
                REQUIRE(arrived[3]);
                REQUIRE(!arrived[4]);
                REQUIRE(arrived[9]);
                REQUIRE(arrived[11]);
                REQUIRE(!arrived[12]);
                REQUIRE(arrived[13]);
                REQUIRE(arrived[15]);
                REQUIRE(arrived[17]);
                REQUIRE(!arrived[18]);
                REQUIRE(arrived[19]);
            }
            SECTION("overly smart scheduling")
            {
                typedef FunctorTraits::function_type function_type;

                auto _f = function_type::make_model(
                    [&arrived](unsigned* wake, unsigned current_time)
                    {
                        arrived.set(*wake);
                        if (*wake < 11 || *wake > 20)
                        {

                        }
                        else
                        {
                            *wake = *wake + 2;
                        }
                    });

                function_type f(&_f);

                scheduler.schedule(11, f);
                scheduler.schedule(3, f);
                scheduler.schedule(9, f);

                scheduler.process(0);
                REQUIRE(!arrived[0]);
                scheduler.process(4);
                REQUIRE(arrived[3]);
                REQUIRE(!arrived[9]);
                scheduler.process(9);
                REQUIRE(arrived[9]);
                scheduler.process(11);
                REQUIRE(arrived[11]);
                scheduler.process(20);
                REQUIRE(!arrived[12]);
                REQUIRE(arrived[13]);
                REQUIRE(!arrived[14]);
                REQUIRE(arrived[15]);
                REQUIRE(!arrived[16]);
            }
        }
        SECTION("stateful")
        {
            embr::internal::layer1::Scheduler<5, StatefulFunctorTraits> scheduler;

            auto f = StatefulFunctorTraits::make_function(
                [&](unsigned* wake, unsigned current)
            {
                arrived.set(*wake);
            });

            scheduler.schedule_now(f);

            scheduler.process(10);

            REQUIRE(arrived.count() == 1);
            REQUIRE(arrived[0]);
        }
        SECTION("std-style dynamic allocated function")
        {
            embr::internal::layer1::Scheduler<5, FunctorTraits> scheduler;

            estd::experimental::function<void(unsigned*, unsigned)> f(
                [&](unsigned* wake, unsigned current_time)
                {
                    arrived.set(*wake);
                });

            scheduler.schedule(5, f);

            scheduler.process(10);

            REQUIRE(arrived[0] == false);
            REQUIRE(arrived[5] == true);
            REQUIRE(arrived[10] == false);
        }
    }
    SECTION("notifications")
    {
        typedef StatefulFunctorTraits traits_type;
        SchedulerObserver<traits_type> o;
        auto s = embr::layer1::make_subject(o);
        embr::internal::layer1::Scheduler<5, traits_type, decltype(s)> scheduler(s);

        auto f = traits_type::make_function(
            [&](unsigned* wake, unsigned current)
            {
                arrived.set(*wake);
                *wake = current + 2;
            });

        scheduler.schedule(5, f);
        scheduler.schedule(99, f); // should never reach this one

        for(traits_type::time_point i = 0; i < 30; i++)
        {
            scheduler.process(i);
        }

        REQUIRE(o.added == 15);
        REQUIRE(o.removed == 13);
    }
    SECTION("traits")
    {
        // Test primarily exists to ensure things compile.
        // Otherwise, it's mostly an estdlib scope kind of test
        SECTION("FunctorTraits")
        {
            typedef embr::internal::scheduler::impl::Function<int> traits_type;
            typedef typename traits_type::value_type control_structure;

            estd::layer1::vector<typename traits_type::value_type, 10> v;
            estd::experimental::function<void(int*, int)> f([&](int* wake, int current)
            {
                arrived.set(*wake);
            });

            control_structure cs(1, f);

            v.emplace_back(1, f);

            // TODO: Somewhat incomplete test
        }
    }
    SECTION("schedule from inside")
    {
        typedef embr::internal::scheduler::impl::Function<int> impl_type;
        typedef typename impl_type::value_type control_structure;

        embr::internal::layer1::Scheduler<5, impl_type> scheduler;

        int rapid_counter = 0;
        int rapid_total = 0;

        auto rapid_f = impl_type::make_function(
            [&](int* wake, int current_time)
            {
                while(rapid_counter-- > 0)
                {
                    ++rapid_total;
                    *wake += 1;
                }
            });

        auto slow_f = impl_type::make_function(
            [&](int* wake, int current_time)
            {
                if(*wake % 30 == 0)
                {
                    rapid_counter = 5;
                    scheduler.schedule(current_time, rapid_f);
                }

                *wake += 10;
            });

        scheduler.schedule(10, slow_f);

        for(int i = 0; i < 70; i++)
        {
            scheduler.process(i);
        }

        REQUIRE(rapid_total == 10);
    }
    SECTION("match")
    {
        typedef estd::layer1::vector<Item, 20> container_type;
        embr::internal::Scheduler<container_type, ItemTraits> scheduler;
        int counter1 = 0, counter2 = 0;

        scheduler.schedule(1, &counter1);
        scheduler.schedule(10, &counter2);

        Item* i = scheduler.match(&counter1);

        REQUIRE(i->event_due == 1);
    }
}
