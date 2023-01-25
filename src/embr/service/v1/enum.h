#pragma once

#include <estd/internal/deduce_fixed_size.h>

namespace embr {

inline namespace service { inline namespace v1 {

struct ServiceBase
{

enum Properties
{
    PROPERTY_STATE = 0,
    PROPERTY_SUBSTATE
};



enum States
{
    Stopped = 0,
    Started,

    Dependency,
    Error,

    //STATES_MAX
};

#define STATES_MAX_BITSIZE 2

enum Substates
{
    // stopped states
    Unstarted,
    Configuring,        ///< pre-start step announcing preliminiry configuration
    Configured,         ///< pre-start step finishing preliminiry configuration
    Finished,
    Restarting,         ///< When restarting, only this is emitted - not Starting
    Starting,           ///< Reflects movement from Stopped::Unstarted to Started::Running - does NOT reflect restart
    Paused,
    Resuming,

    // running states
    Running,
    Connecting,
    Online,
    Disconnecting,
    Offline,
    Degraded,
    Pausing,
    Stopping,
    Resetting,

    // error states
    ErrConfig,         ///< service configuration error, usually on startup or configuring
    ErrMemory,         ///< service ran out of memory, or detected memory corruption
    ErrUnspecified,    ///< internal error code was not recognized or provided

    //SUBSTATES_MAX
};

// NOTE: Not using *_MAX - 1 values because compiler warns us sometimes that
// bit count isn't high enough, which is true since _MAX takes up one more slot.

typedef estd::integral_constant<unsigned,
    estd::internal::deduce_bit_count<Error>::value> state_bitsize;

typedef estd::integral_constant<unsigned,
    estd::internal::deduce_bit_count<ErrUnspecified>::value> substate_bitsize;

struct state_result
{
    const States state : state_bitsize::value;
    const Substates substate : substate_bitsize::value;

    operator bool() const { return state == Started && substate == Running; }
};

};


}}

}
