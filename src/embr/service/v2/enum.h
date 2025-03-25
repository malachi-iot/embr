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

    // yields 32 possible substates per category, allowing us to stay within 8 bits
    static constexpr unsigned separator = 6;

    enum substates
    {
        // stopped states
        Unstarted,          ///< has never started
        Configuring,        ///< pre-start step announcing preliminiry configuration
        Configured,         ///< pre-start step finishing preliminiry configuration
        Finished,           ///< finished running, now in 'off' state
        Restarting,         ///< When restarting, only this is emitted - not Starting
        Starting,           ///< Reflects movement from Stopped::Unstarted to Started::Running - does NOT reflect restart
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
        Resetting,          ///< When restarting, this is the transition out of running -> stopped state
        Sleeping,           ///< In process of entering sleep mode
        Sparse,             ///< Sparse services default to this special running state - 90% overlap with "indeterminate"

        // error states
        /// service configuration error, usually on startup or configuring
        ErrConfig = Error << separator,
        ErrMemory,         ///< service ran out of memory, or detected memory corruption
        ErrTimeout,
        ErrUnspecified,    ///< internal error code was not recognized or provided

        SUBSTATES_MAX
    };

    substates substate_ = Unstarted;

    [[nodiscard]] constexpr substates substate() const { return substate_; }
    [[nodiscard]] constexpr states state() const
    {
        return static_cast<states>(substate_ >> separator);
    }

#ifndef UNIT_TESTING
protected:
#endif
    void substate(substates s)
    {
        substate_ = s;
    };
};

}

using service = detail::service;

}}

}

