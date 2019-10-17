#include <catch.hpp>

#include <embr/observer.h>

static int expected;

struct event_1
{
    int data;
};


struct event_2
{
    int data;
};


struct event_3
{
    int data;
    int output = 0;
};

struct id_event
{
    int id;
};

struct noop_event {};

static int counter = 0;

class StatelessObserver
{
public:
    static void on_notify(int val)
    {
        REQUIRE(val == expected);
        counter++;
    }

    static void on_notify(event_1 val, const int& context)
    {

    }
};

class IObserver
{
public:
    virtual void on_notify(event_1 n) = 0;
};

class FakeBase {};

static int unique_counter = 0;


class StatefulObserver : public FakeBase
{
public:
    static constexpr int default_id() { return 0x77; }

    int unique = unique_counter++;
    int id;
    int counter = 0;
    int context_counter = 0;

    StatefulObserver() : id(default_id()) {}

    explicit StatefulObserver(int id) : id(id) {}

    void on_notify(int val)
    {
        REQUIRE(val == expected);
    }

    void on_notify(event_3 e, event_3& context)
    {
        REQUIRE(e.data == expected);

        context.output = default_id();

        context_counter++;
    }


    void on_notify(event_1 e)
    {
        REQUIRE(e.data == expected);
    }

    void on_notify(const event_2& e)
    {
        REQUIRE(e.data == expected);
    }

    void on_notify(id_event e)
    {
        counter++;

        REQUIRE(e.id == id);
    }

    static void on_notify(event_1 val, const int& context)
    {

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


TEST_CASE("observer")
{
    SECTION("void subject")
    {
        embr::void_subject vs;

        vs.notify(3);
    }
    SECTION("next-gen tuple based observer")
    {
        counter = 0;
        expected = 3;

        SECTION("layer0")
        {
            SECTION("raw")
            {
                embr::layer0::subject<
                        StatelessObserver,
                        StatelessObserver> s;

                int sz = sizeof(s);

                s.notify(3);
                REQUIRE(counter == 2);
                s.notify(event_1 { 3 }); // goes nowhere
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

            const StatefulObserver& so = s.get<1>();

            REQUIRE(so.id == StatefulObserver::default_id());

            int sz = sizeof(s);

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

            SECTION("make_subject")
            {
                StatelessObserver o1;
                StatefulObserver o2(0x777);

                id_event e;

                e.id = 0x777;

                auto s = embr::layer1::make_subject(
                            o1,
                            o2);

                // StatelessObserver will just ignore it
                s.notify(e);

                REQUIRE(o2.counter == 1);
            }
            SECTION("proxy")
            {
                embr::layer1::internal::observer_proxy<decltype(s)> op(s);

                op.on_notify(noop_event{});
            }
            SECTION("proxy + make_subject")
            {
                embr::layer1::internal::observer_proxy<decltype(s)> op(s);
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
                void* ctx;

                e2.data = expected;

                REQUIRE(o.context_counter == 0);
                // FIX: Following not working because 'so' got copied instead of referenced
                //REQUIRE(so.context_counter == 0);

                s2.notify(e2, e2);

                REQUIRE(o.context_counter == 1);
                REQUIRE(so.context_counter == 1);
            }
        }
    }
}
