#pragma once

#include <estd/internal/platform.h>

#include "fwd.h"

namespace embr { namespace service { namespace v2 {

// DEBT: Refactor to be embr::service::detail::v2
namespace detail {

struct service
{
    enum states
    {
        Stopped = 0,
        Started,

        Error,

        STATES_MAX
    };

    // yields 32 possible substates per category, allowing us to stay well within 8 bits
    static constexpr unsigned separator = 5;

    enum substates
    {
        // stopped states
        Unstarted,          ///< has never started
        Configuring,        ///< pre-start step announcing preliminiry configuration
        Configured,         ///< pre-start step finishing preliminiry configuration
        Finished,           ///< finished running, now in 'off' state
        Restarting,         ///< When restarting, only this is emitted - not Starting
        Starting,           ///< Reflects movement from Stopped::Unstarted to Started::Running - does NOT reflect restart
        Sleeping,           ///< Has dozed off and is currently asleep
        Paused,
        Resuming,
        Waking,             ///< This precedes a Starting state when coming out of a sleeping state
        Dependency,         ///< Waiting on a dependency before we can run

        // running states
        Running = Started << separator,
        Connecting,
        Online,
        Disconnecting,
        Offline,
        Degraded,
        Pausing,
        Stopping,
        Resetting,          ///< When restarting, this is the transition out of running -> restarting state
        DozingOff,          ///< In process of entering sleep mode
        Sparse,             ///< Sparse services default to this special running state - 90% overlap with "indeterminate"

        // error states

        /// error code was not recognized or provided
        ErrUnspecified = Error << separator,
        /// service configuration error, usually on startup or configuring
        ErrConfig,
        ErrMemory,         ///< service ran out of memory, or detected memory corruption
        ErrTimeout,

        SUBSTATES_MAX
    };
};

// Our convention is upper bits = major state and lower bits = transition state
template <class Impl>
class state_machine : public Impl
{
    using base_type = Impl;
    using base_type::separator;

public:
    using typename base_type::states;
    using typename base_type::substates;

    [[nodiscard]] constexpr substates substate() const { return substate_; }
    [[nodiscard]] constexpr states state() const
    {
        return static_cast<states>(substate_ >> separator);
    }

    constexpr state_machine(substates substate = {}) :
        substate_{substate}
    {}

#ifndef UNIT_TESTING
protected:
#endif
    // DEBT: Refactor to (optionally) use a packed struct or similar so that a user area
    // is available out of the unused bits of substate_

    substates substate_;

    ESTD_CPP_CONSTEXPR(17) void substate(substates s)
    {
        substate_ = s;
    }

    ESTD_CPP_CONSTEXPR(17) void state(substates s)
    {
        substate_ = s;
    }

    ESTD_CPP_CONSTEXPR(17) void state(states s)
    {
        substate_ = static_cast<substates>(s << separator);
    }
};

}

using service = detail::state_machine<detail::service>;

}}}

