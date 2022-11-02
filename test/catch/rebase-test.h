#pragma once

#include "scheduler-test.h"

namespace test { namespace rebase {

struct Item : test::scheduler::Item
{
    typedef test::scheduler::Item base_type;
    typedef int time_point;

    void rebase(int v)
    {
        event_due_ -= v;
    }

    ESTD_CPP_FORWARDING_CTOR(Item)
};


struct ChronoItem : test::scheduler::Item3ControlStructure1
{
    typedef test::scheduler::Item3ControlStructure1 base_type;
    typedef typename base_type::time_point time_point;

    void rebase(time_point v)
    {
        // TODO: We need a duration here
        //base_type::t -= v;
    }

    ESTD_CPP_FORWARDING_CTOR(ChronoItem)
};

}}