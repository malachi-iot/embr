#pragma once

#include "../observer.h"

namespace embr { namespace experimental {

template <class T, T id_>
struct PropertyTraits;


namespace event {

template <typename T, int id = -1>
struct PropertyChanged;


template <typename T>
struct PropertyChanged<T, -1>
{
    const int id_;
    int id() const { return id_; }

    const T value;

    PropertyChanged(int id, T value) :
        id_{id},
        value{value}
    {}

    template <int id_>
    PropertyChanged(const PropertyChanged<T, id_>& copy_from) :
        id_{copy_from.id()},
        value{copy_from.value}
    {}
};

template <typename T, int id_>
struct PropertyChanged
{
    const T value;

    static constexpr int id() { return id_; }

    PropertyChanged(T value) : value{value} {}
};


}

// Copy/pasted/adapter from chariot project
namespace service {

enum Properties
{
    PROPERTY_STATE,
    PROPERTY_SUBSTATE
};



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

    template <int id, typename T, class TContext>
    void fire_changed2(T v, TContext& context)
    {
        subject_type::notify(event::PropertyChanged<T, id>{v}, context);
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
    template <class TContext>
    void state(service::States s, TContext& context)
    {
        if(s != state_.service_)
        {
            state_.service_ = s;

            base_type::template fire_changed2<service::PROPERTY_STATE>(s, context);
        }
    }


    void state(service::States s)
    {
        state(s, *this);
    }

    template <class TContext>
    void state(service::Substates s, TContext& context)
    {
        if(s != state_.service_substate_)
        {
            state_.service_substate_ = s;

            base_type::template fire_changed2<service::PROPERTY_SUBSTATE>(s, context);
        }
    }

    void state(service::Substates s)
    {
        state(s, *this);
    }

    template <class TContext>
    void state(service::States s, service::Substates ss, TContext& context)
    {
        if(s != state_.service_)
        {
            state_.service_ = s;
            state_.service_substate_ = ss;

            base_type::template fire_changed2<service::PROPERTY_STATE>(s, context);
        }
    }


    void state(service::States s, service::Substates ss)
    {
        state(s, ss, *this);
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


template <class TImpl, class TSubject = embr::void_subject>
class Service2 : public Service<TSubject>,
    TImpl
{
    typedef Service<TSubject> base_type;
    typedef TImpl impl_type;

protected:
    //impl_type impl_;

    impl_type& impl() { return *this; }

    void state(service::States s)
    {
        base_type::state(s, impl());
    }

    void state(service::Substates s)
    {
        base_type::state(s, impl());
    }

    void state(service::States s, service::Substates ss)
    {
        base_type::state(s, ss, impl());
    }

public:
    Service2() = default;
    Service2(TSubject&& subject) : base_type(std::move(subject))
    {}

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