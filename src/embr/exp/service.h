#pragma once

#include "property.h"


namespace embr { namespace experimental {

namespace event {

struct Registration
{
    const char* name;
    const char* instance;
};


}


// Copy/pasted/adapter from chariot project
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

namespace impl {

struct Service;

}




namespace impl {

struct Service : event::owner_tag
{
    typedef Service this_type;

    constexpr static const char* name() { return "Generic service"; }
    constexpr static const char* instance() { return ""; }

    union
    {
        struct
        {
            service::States service_: STATES_MAX_BITSIZE;
            service::Substates service_substate_: 6;

            service::States child1 : STATES_MAX_BITSIZE;
            service::States child2 : STATES_MAX_BITSIZE;
            service::States child3 : STATES_MAX_BITSIZE;

        } state_;

        unsigned raw = 0;
    };

    struct id
    {
        struct state :
            EMBR_PROPERTY_TRAITS_BASE(this_type, state_.service_, service::PROPERTY_STATE, "state");

        struct substate :
            EMBR_PROPERTY_TRAITS_BASE(this_type, state_.service_substate_, service::PROPERTY_SUBSTATE, "substate");
    };

    template <class TSubject, class TImpl = this_type>
    struct responder //: PropertyNotifier<TSubject&>
    {
        TImpl& impl;
        TSubject& subject;

        constexpr responder(TImpl& impl, TSubject& subject) :
            impl(impl), subject(subject) {}

        operator TImpl& () { return impl; }
    };

protected:
    bool start() { return true; }
    bool stop() { return true; }
};

}

template <>
struct PropertyTraits<service::Properties, service::PROPERTY_STATE>
{
    typedef service::States value_type;

    static constexpr const char* name() { return "Service State"; }
};

template <>
struct PropertyTraits<service::Properties, service::PROPERTY_SUBSTATE>
{
    typedef service::Substates value_type;

    static constexpr const char* name() { return "Service sub-state"; }
};

EMBR_PROPERTY_DECLARATION2(impl::Service, state)
EMBR_PROPERTY_DECLARATION2(impl::Service, substate)


template <class TImpl = impl::Service, class TSubject = embr::void_subject>
class Service : public PropertyNotifier<TSubject>,
    TImpl
{
    typedef PropertyNotifier<TSubject> base_type;
    //using subject_base = typename base_type::subject_type;

    template <int id, typename T>
    void service_setter(T v)
    {
        base_type::template setter<id, impl::Service>(v, impl());
    }

protected:
    typedef TImpl impl_type;
    //impl_type impl_;

    impl_type& impl() { return *this; }

    template <int id, typename T>
    void setter(T v)
    {
        //impl_type& context = impl();
        TSubject& subject = *this;
        typename impl_type::template responder<TSubject> context{impl(), subject};
        base_type::template setter<id, impl_type>(v, context);
    }

protected:
    void state(service::States s)
    {
        service_setter<service::PROPERTY_STATE>(s);
    }


    void state(service::Substates s)
    {
        service_setter<service::PROPERTY_SUBSTATE>(s);
    }

    void state(service::States s, service::Substates ss)
    {
        if(s != impl().state_.service_)
        {
            base_type::template fire_changing<impl::Service::id::state>(impl().state_.service_, s, impl());

            impl().state_.service_ = s;
            impl().state_.service_substate_ = ss;

            base_type::template fire_changed3<experimental::impl::Service::id::state>(s, impl());
            //base_type::template fire_changed2<service::PROPERTY_STATE>(s, context);
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
    {
        base_type::notify(event::Registration{impl().name(), impl().instance()}, impl());
    }

    void start()
    {
        state(service::Starting);
        if(impl_type::start())
        {
            state(service::Started, service::Running);
        }
    }
};


template <template <class> class TService, class TSubject>
TService<TSubject> make_service(TSubject&& subject)
{
    return TService<TSubject>(std::move(subject));
}


}}
