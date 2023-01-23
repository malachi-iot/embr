#pragma once

namespace embr { namespace experimental {

namespace impl {

struct Service;

}


// Copy/pasted/adapted from chariot project
namespace service {

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
    Starting,
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
};

}

template <class TImpl = impl::Service, class TSubject = embr::void_subject>
class Service;

template <class TImpl, class TSubject = embr::void_subject>
class ServiceSpec;


}}