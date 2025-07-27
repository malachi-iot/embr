#pragma once

#include <estd/internal/platform.h>

// At this time, mostly used to support Scheduler things

namespace embr { namespace internal {

struct MutexContext
{
    const bool use_mutex_ : 1;
    const bool in_isr_ : 1;

    constexpr bool use_mutex() const { return use_mutex_; }
    constexpr bool in_isr() const { return in_isr_; }

    constexpr MutexContext(bool in_isr, bool use_mutex = true) :
        use_mutex_(use_mutex),
        in_isr_(in_isr)
    {

    }
};


struct noop_mutex
{
    static ESTD_CPP_CONSTEXPR(14) void lock(MutexContext = {false}) {}

    static ESTD_CPP_CONSTEXPR(14) void unlock(MutexContext = {false}) {}
};


}}
