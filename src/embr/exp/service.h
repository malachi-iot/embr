#pragma once

#include "../observer.h"

namespace embr { namespace experimental {

namespace event {


template <typename T>
struct PropertyChanged
{
    int id;
    const T value;
};

}

// Copy/pasted/adapter from chariot project
namespace service {

enum States
{
    Stopped = 0,
    Started,

    Dependency,
    Error,
};

enum Substates
{
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

    // stopped states
    Unstarted,
    Configuring,        ///< pre-start step announcing preliminiry configuration
    Configured,         ///< pre-start step finishing preliminiry configuration
    Finished,
    Starting,
    Paused,
    Resuming,

    // error states
    ErrConfig,         ///< service configuration error, usually on startup or configuring
    ErrMemory,         ///< service ran out of memory, or detected memory corruption
    ErrUnspecified,    ///< internal error code was not recognized or provided
};

}

template <class TSubject = embr::void_subject>
class PropertyNotifier : public TSubject
{
    typedef TSubject subject_type;

protected:
    template <typename T, class TContext>
    void fire_changed(int id, T v, TContext& context)
    {
        subject_type::notify(event::PropertyChanged<T>{id, v}, context);
    }

public:
    PropertyNotifier() = default;
    PropertyNotifier(TSubject&& subject) : subject_type(std::move(subject))
    {}
};

template <class TSubject = embr::void_subject>
class Service : public PropertyNotifier<TSubject>
{
    typedef PropertyNotifier<TSubject> base_type;

    struct
    {
        service::States service_ : 4;
        service::Substates service_substate_ : 6;

    }   state_;

protected:
    void state(service::States s)
    {
        if(s != state_.service_)
        {
            state_.service_ = s;

            base_type::fire_changed(0, s, *this);
        }
    }


    void state(service::Substates s)
    {
        if(s != state_.service_substate_)
        {
            state_.service_substate_ = s;

            base_type::fire_changed(0, s, *this);
        }
    }

    void state(service::States s, service::Substates ss)
    {
        if(s != state_.service_)
        {
            state_.service_ = s;
            state_.service_substate_ = ss;

            base_type::fire_changed(0, s, *this);
        }
    }



    template <class F>
    void start(F&& f)
    {
        state(service::Starting);
        if(f())
        {
            state(service::Started, service::Running);
        }
    }

    template <class F>
    void stop(F&& f)
    {
        state(service::Stopping);
        f();
        state(service::Stopped, service::Finished);
    }

public:
    Service() = default;
    Service(TSubject&& subject) : base_type(std::move(subject))
    {}
};

}}