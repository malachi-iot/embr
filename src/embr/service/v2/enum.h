#pragma once

#include "fwd.h"

namespace embr {

inline namespace service { namespace v2 {

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

    [[nodiscard]] constexpr substates substate() const { return substate_; }
    [[nodiscard]] constexpr states state() const
    {
        return static_cast<states>(substate_ >> separator);
    }

#ifndef UNIT_TESTING
protected:
#endif
    substates substate_ = Unstarted;

    void substate(substates s)
    {
        substate_ = s;
    };

    void state(substates s)
    {
        substate_ = s;
    }

    void state(states s)
    {
        substate_ = static_cast<substates>(s << separator);
    }
};

}

using service = detail::service;

}}

}

