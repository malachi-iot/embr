#pragma once

#include <estd/chrono.h>
#include <estd/limits.h>

#include "fwd.h"

#include <estd/internal/macro/push.h>

// DEBT: namespace needs work
namespace embr { namespace internal { namespace scheduler { namespace impl {

template <typename TInt>
struct TimePointTraits
{
    typedef TInt time_point;    ///< Core time_point on which scheduling is based
    typedef TInt int_type;      ///< Underlying int type - useful for complex time_points like chrono's
    typedef TInt duration;      ///< Type to do additions, subtractions, etc. on time_point - mimic/aliases chrono

    // EXPERIMENTAL
    static constexpr bool is_chrono() { return false; }

    static constexpr int_type duration_zero() { return 0; }
    static constexpr int_type time_point_max() { return estd::numeric_limits<int_type>::max(); }
};

template <typename Rep, typename Period>
struct TimePointTraits<estd::chrono::duration<Rep, Period> >
{
    typedef estd::chrono::duration<Rep, Period> duration;
    typedef duration time_point;
    typedef Rep int_type;

    static constexpr bool is_chrono() { return true; }

    static constexpr duration duration_zero() { return duration::zero(); }
    static constexpr time_point time_point_max() { return time_point::max(); }
};

template <typename TClock, typename TDuration>
struct TimePointTraits<estd::chrono::time_point<TClock, TDuration> >
{
    typedef TDuration duration;
    typedef estd::chrono::time_point<TClock, TDuration> time_point;
    typedef typename TDuration::rep int_type;

    static constexpr bool is_chrono() { return true; }

    static constexpr duration duration_zero() { return duration::zero(); }
    static constexpr time_point time_point_max() { return time_point::max(); }

    static time_point now() { return TClock::now(); }
};


#if FEATURE_STD_CHRONO
template <typename TClock, typename TDuration>
struct TimePointTraits<std::chrono::time_point<TClock, TDuration> >
{
    typedef TDuration duration;
    typedef std::chrono::time_point<TClock, TDuration> time_point;
    typedef typename TDuration::rep int_type;

    static constexpr bool
    is_chrono() { return true; }

    static constexpr duration duration_zero() { return duration::zero(); }
    static constexpr time_point time_point_max() { return time_point::max(); }

    static time_point now() { return TClock::now(); }
};
#endif

}}}}
