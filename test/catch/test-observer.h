#pragma once

#include <estd/cstdint.h>

extern int expected;

class FakeBase {};

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

struct event_to_convert
{
    const int data;
};



struct id_event
{
    int id;
};

struct sequence_event : id_event {};


// TODO: Since we sorta suspect event conversion of not playing nice with
// notify_helper, prepping this guy
struct converting_event
{
    const int data;

    constexpr converting_event(event_to_convert e) :
        data(e.data) {}
};




class StatefulObserver : public FakeBase
{
    static int unique_counter;

public:
    static constexpr int default_id() { return 0x77; }

    typedef int32_t int_type;

    int_type unique = unique_counter++;
    int_type id;
    int_type counter = 0;
    int_type context_counter = 0;
    int_type converting_counter = 0;
    int_type last_int = -1;

    StatefulObserver() : id(default_id()) {}

    explicit StatefulObserver(int id) : id(id) {}

    void on_notify(int val)
    {
        REQUIRE(val == expected);
        last_int = val;
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

    void on_notify(converting_event e)
    {
        converting_counter += e.data;
    }


    void on_notify(id_event e)
    {
        counter++;

        REQUIRE(e.id == id);
    }

    void on_notify(sequence_event e, sequence_event& context)
    {
        counter++;

        REQUIRE(unique > context.id);

        context.id = unique;
    }

    static void on_notify(event_1 val, const int& context)
    {

    }
};


