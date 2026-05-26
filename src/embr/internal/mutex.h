#pragma once

#if __cpp_lib_concepts
#include <concepts>
#endif

#include <estd/internal/platform.h>

// At this time, mostly used to support Scheduler things

#if __cpp_concepts
namespace embr::internal::concepts {

template <class T>
concept Mutex = requires(T t)
{
    // Should we bother with https://github.com/malachi-iot/estdlib/issues/46 ?
#if __cpp_lib_concepts
    { t.lock() } -> std::same_as<bool>;
#endif
    t.unlock();
};


}
#endif

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



// DEBT: This ought to go into a port/platform area
// Primarily used by thunk/msg-bipbuf

struct noop_mutex
{
    static constexpr bool lock(MutexContext = {false}) { return true; }

    static ESTD_CPP_CONSTEXPR(14) void unlock(MutexContext = {false}) { }
};


}}
