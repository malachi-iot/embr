#pragma once

#include <estd/internal/deduce_fixed_size.h>

namespace embr {

inline namespace service { inline namespace v1 {

struct ServiceEnum
{

enum Properties
{
    STATE = 0,
    SUBSTATE,
    USER
};



enum States
{
    Stopped = 0,
    Started,

    Error,

    //STATES_MAX
};

#define STATES_MAX_BITSIZE 2

// NOTE: Not used yet, anticipating scenarios where we want to reduce the bit size of Substates,
// this could be helpful
#define FEATURE_EMBR_SERVICE_RESTART 1
#define FEATURE_EMBR_SERVICE_PAUSE 1
#define FEATURE_EMBR_SERVICE_SLEEP 1

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
    Waking,             ///< This precedes a Starting state when coming out of a sleeping state
    Dependency,         ///< Waiting on a dependency before we can run

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
    Sleeping,           ///< In process of entering sleep mode

    // error states
    ErrConfig,         ///< service configuration error, usually on startup or configuring
    ErrMemory,         ///< service ran out of memory, or detected memory corruption
    ErrUnspecified,    ///< internal error code was not recognized or provided

    //SUBSTATES_MAX
};


struct bitsize
{
    // NOTE: Not using *_MAX - 1 values because compiler warns us sometimes that
    // bit count isn't high enough, which is true since _MAX takes up one more slot.

    typedef estd::integral_constant<unsigned,
        estd::internal::deduce_bit_count<Error>::value> state;

    typedef estd::integral_constant<unsigned,
        estd::internal::deduce_bit_count<ErrUnspecified>::value> substate;

    typedef estd::integral_constant<unsigned, 6> user;
};

};

struct ServiceBase : ServiceEnum
{
    struct state_result
    {
        // DEBT: Frustratingly can't use const here because we fall into that operator=
        // trap
        States state : bitsize::state::value;
        Substates substate : bitsize::substate::value;

        operator bool() const { return state == Started && substate == Running; }

        static constexpr state_result started()
        {
            return state_result{Started, Running};
        }
    };

    // pre-processing (such as wake, resume, etc.) preceding a 'starting' event
    static inline void on_starting() {  }

    //template <class TSubject, class TImpl>
    //static constexpr state_result on_start(runtime<TSubject, TImpl>&)
    static constexpr state_result on_start()
    {
        return state_result::started();
    }

};


}}

}
