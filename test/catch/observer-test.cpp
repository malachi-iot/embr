#include <catch2/catch.hpp>

#include <embr/observer.h>

#include "test-observer.h"

int expected;

struct noop_event {};

static int counter = 0;
static int allow_counter = 0;   // increases every time allow is evaluated

class StatelessObserver
{
public:
    static void on_notify(int val)
    {
        REQUIRE(val == expected);
        counter++;
    }

    /*
     * just for experimentation
    template <class TContext>
    static void on_notify(event_1 val, TContext&)
    {

    } */

    static void on_notify(event_1 val, const int& context)
    {
        counter += val.data;
    }

    template <class TEvent>
    static bool allow_notify(TEvent)
    {
        ++allow_counter;
        return true;
    }

    bool allow_notify(event_1 val, const int& context) const
    {
        ++allow_counter;
        return context != 8;
    }
};


struct OtherStatefulObserver
{
    char buf[10];

    OtherStatefulObserver()
    {
        buf[0] = 1;
        buf[1] = 2;
        buf[2] = 3;
        buf[3] = 4;
    }

    void on_notify(int val)
    {
        REQUIRE(val == expected);
    }
};

StatefulObserver so1;
using so1_type = estd::integral_constant<StatefulObserver*, &so1>;


int StatefulObserver::unique_counter = 0;


TEST_CASE("observer")
{
    SECTION("general compiler sanity checks")
    {
        // for debian x64 gcc
        REQUIRE(sizeof(int) == 4);
        REQUIRE(sizeof(StatefulObserver) == sizeof(int) * 6);
    }
    SECTION("void subject")
    {
        embr::void_subject vs;

        vs.notify(3);
    }
    SECTION("next-gen tuple based observer")
    {
        // Conveniently, catch re-runs this so that these are reinitialized per section
        counter = 0;
        expected = 3;
        allow_counter = 0;

        SECTION("layer0")
        {
            embr::layer0::subject<
                StatelessObserver,
                StatelessObserver> s;

            SECTION("raw")
            {
                int sz = sizeof(s);

                s.notify(3);
                REQUIRE(counter == 2);
                REQUIRE(allow_counter == 2);

                s.notify(event_1 { 3 }); // goes nowhere due to lack of context to match

                REQUIRE(counter == 2);
                REQUIRE(allow_counter == 2);    // Never increases because notify wasn't found in the first place

                REQUIRE(sz == 1);

                int context = 7;

                // now we have a context match it will proceed
                s.notify(event_1 { 3 }, context);

                // remember, there's two of them - so 2 + 3 + 3 == 8
                REQUIRE(counter == 8);
                REQUIRE(allow_counter == 4);

                context = 8;

                // value 8 is a special context which filters out do_notify via
                // experimental allow_notify mechanism
                s.notify(event_1 { 3 }, context);

                // filtered out notify via allow counter, so no counter increase
                REQUIRE(counter == 8);
                REQUIRE(allow_counter == 6);
            }
            SECTION("append (experimental)")
            {
                typedef decltype(s)::append<StatelessObserver> subject_type;
                subject_type s2;

                REQUIRE(counter == 0);
                s2.notify(3);
                REQUIRE(counter == 3);
                REQUIRE(allow_counter == 3);
            }
            SECTION("integral_constant")
            {
                using s2_type = decltype(s)::append<so1_type>;

                s2_type{}.notify(3);

                REQUIRE(so1.last_int == 3);
            }
        }
        SECTION("layer1")
        {
            embr::layer1::subject<
                    StatelessObserver,
                    StatefulObserver,
                    StatelessObserver,
                    StatelessObserver
                    > s;

            typedef decltype(s) subject_type;

            const StatefulObserver& so = estd::get<1>(s.observers());

            REQUIRE(so.id == StatefulObserver::default_id());

            s.notify(3);

            // count 3 times, once per stateless observer
            REQUIRE(counter == 3);
            s.notify(event_1 { 3 });

            event_3 ctx;

            // Notify is gonna evaluate whether 'data' matches 'expected', which
            // is 3 in this case
            ctx.data = expected;

            s.notify(ctx, ctx);

            // context should be modified by stateful observer

            REQUIRE(ctx.output == StatefulObserver::default_id());

            s.notify(event_to_convert{4});

            REQUIRE(so.converting_counter == 4);

            SECTION("size evaluation")
            {
                int sz = sizeof(s);

                // for debian x64
                // Stateful = 20
                REQUIRE(sz == sizeof(StatefulObserver));

                embr::layer1::subject<
                    StatefulObserver
                > s_smallest_stateful;

                embr::layer1::subject<
                    StatefulObserver,
                    StatelessObserver
                > s_next_smallest_stateful;

                estd::tuple<StatefulObserver, StatelessObserver> t_next_smallest_stateful;

                REQUIRE(sizeof(s_smallest_stateful) == sizeof(StatefulObserver));
            }
            SECTION("make_subject")
            {
                StatelessObserver o1;
                StatefulObserver o2(0x777);

                id_event e;

                e.id = 0x777;

                REQUIRE(o2.counter == 0);

                auto s = embr::layer1::make_subject(
                            o1,
                            o2);

                // StatelessObserver will just ignore it
                s.notify(e);

                REQUIRE(o2.counter == 1);

                SECTION("size evaluation")
                {
                    int sz = sizeof(s);

                    // for debian x64
                    REQUIRE(sz == 16);
                }
            }
            SECTION("proxy")
            {
                embr::layer1::observer_proxy<decltype(s)> op(s);

                op.on_notify(noop_event{});
            }
            SECTION("make_observer_proxy")
            {
                auto op = embr::layer1::make_observer_proxy(s);

                event_3 e;

                e.data = expected;

                REQUIRE(so.context_counter == 1);

                op.on_notify(e, e);

                REQUIRE(so.context_counter == 2);
            }
            SECTION("proxy + make_subject")
            {
                embr::layer1::observer_proxy<subject_type> op(s);
                StatefulObserver o;

                id_event e;

                e.id = StatefulObserver::default_id();

                REQUIRE(o.counter == 0);

                auto s2 = embr::layer1::make_subject(
                        op,
                        o);

                s2.notify(e);

                REQUIRE(o.counter == 1);

                // TODO: Would be nice to do a expect-fails condition here, don't know if catch does that
                // we'd look for s2.notify to fail if it's not default_id

                event_3 e2;

                e2.data = expected;

                REQUIRE(o.context_counter == 0);
                REQUIRE(so.context_counter == 1);

                s2.notify(e2, e2);

                REQUIRE(o.context_counter == 1);
                REQUIRE(so.context_counter == 2);
            }
            SECTION("Notification order")
            {
                StatefulObserver o1, o2, o3;

                //auto s = embr::layer1::make_subject(o3, o2, o1);  // this should fail
                auto s = embr::layer1::make_subject(o1, o2, o3);

                sequence_event e;

                e.id = 0;

                s.notify(e, e);

                REQUIRE(o1.counter == 1);
                REQUIRE(o2.counter == 1);
                REQUIRE(o3.counter == 1);
            }
            SECTION("append (experimental)")
            {
                using t1_type = estd::tuple<int, float>;
                using t2_type = estd::tuple<estd::monostate, const char*>;
                using cat_helper_type = embr::experimental::tuple_cat_helper<t1_type, t2_type>;

                t1_type t1;
                t2_type t2;

                // FIX: Not ready yet, apply seems to need some work
                //cat_helper_type::tester(t1, t2);
            }
        }
    }
    SECTION("experimental")
    {
        SECTION("from delegate")
        {
            int counter = 0;

            auto o1 = embr::experimental::make_delegate_observer([&counter](int e)
            {
                counter += e;
            });
            auto o2 = embr::experimental::make_delegate_observer([&counter](const char* e)
            {
                counter += 100;
            });

            auto s = embr::layer1::make_subject(o1, o2);

            s.notify(1);
            s.notify(3);

            REQUIRE(counter == 4);

            s.notify("hello");

            REQUIRE(counter == 104);
        }
    }
}
