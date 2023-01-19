#pragma once

#include "../observer.h"

namespace embr { namespace experimental {

template <class T, T id_>
struct PropertyTraits;

template <class T, int id_>
struct PropertyTraits2;



namespace event {

struct traits_tag {};

struct owner_tag {};

template <class T, int id_>
struct traits_base : traits_tag
{
    typedef T value_type;
    static constexpr int id() { return id_; }
};

template <typename T, int id = -1, class enabled = void>
struct PropertyChanged;


template <typename T, int id = -1, class enabled = void>
struct PropertyChanging;

struct Registration
{
    const char* name;
    const char* instance;
};


template <typename T>
struct PropertyChanged<T, -1, typename estd::enable_if<
    !estd::is_base_of<traits_tag, T>::value &&
    !estd::is_base_of<owner_tag, T>::value
    >::type>
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

    /*
    // NOTE: Won't work since traits_base isn't an EXACT match but rather a base class
    template <int id__>
    PropertyChanged(const PropertyChanged<traits_base<T, id__> >& copy_from) :
        id_{copy_from.id()},
        value(copy_from.value)
    {

    }
     */

    // For converting traits flavor to baseline (this one) flavor
    template <class T2,
        typename estd::enable_if<
            estd::is_base_of<traits_tag, T2>::value &&
            estd::is_same<typename T2::value_type, T>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<T2, -1>& copy_from) :
        id_{copy_from.id()},
        value(copy_from.value)
    {}
};

template <typename T, int id_>
struct PropertyChanged<T, id_, typename estd::enable_if<
    !estd::is_base_of<traits_tag, T>::value &&
    !estd::is_base_of<owner_tag, T>::value &&
    id_ >= 0>::type>
{
    const T value;

    static constexpr int id() { return id_; }

    PropertyChanged(T value) : value{value} {}
};

template <typename T>
struct PropertyChanged<T, -1, typename estd::enable_if<
    estd::is_base_of<traits_tag, T>::value
    >::type>
{
    typedef typename T::value_type value_type;

    const value_type value;

    static constexpr int id() { return T::id(); }
    static const char* name() { return T::name(); }

    PropertyChanged(value_type value) : value{value} {}
};


template <typename TTraits>
struct PropertyChanging<TTraits, -1, typename estd::enable_if<
    estd::is_base_of<traits_tag, TTraits>::value
    >::type> :
    TTraits
{
    typedef typename TTraits::value_type value_type;

    const value_type old_value;
    const value_type new_value;

    PropertyChanging(value_type old, value_type new_value) :
        old_value{old},
        new_value{new_value}
    {}
};


template <typename TOwner, int id_>
struct PropertyChanged<TOwner, id_, typename estd::enable_if<
    estd::is_base_of<owner_tag, TOwner>::value
    >::type> : PropertyTraits2<TOwner, id_>
{
    typedef PropertyTraits2<TOwner, id_> traits;
    typedef typename traits::value_type value_type;

    TOwner* owner;
    const value_type value;

    PropertyChanged(TOwner* owner, value_type v) : owner(owner), value(v) {}
};


template <typename TOwner>
struct PropertyChanged<TOwner, -1, typename estd::enable_if<
    estd::is_base_of<owner_tag, TOwner>::value
    >::type>
{
    TOwner* owner;
    const int id_;

    int id() const { return id_; }

    PropertyChanged(TOwner* owner, int v) : owner(owner), id_(v) {}

    template <int id>
    PropertyChanged(const PropertyChanged<TOwner, id>& copy_from) :
        owner(copy_from.owner),
        id_(copy_from.id())
    {}
};


template <typename TOwner, int id_>
struct PropertyChanging<TOwner, id_, typename estd::enable_if<
    estd::is_base_of<owner_tag, TOwner>::value
>::type> : PropertyTraits2<TOwner, id_>
{
    typedef PropertyTraits2<TOwner, id_> traits_type;
    typedef typename traits_type::value_type value_type;

    TOwner* owner;
    const value_type old_value;
    const value_type new_value;

    PropertyChanging(TOwner* owner, value_type old, value_type new_value) :
        owner(owner),
        old_value{old},
        new_value{new_value}
    {}
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

namespace impl {

struct Service;

}

template <>
struct PropertyTraits2<impl::Service, service::PROPERTY_STATE> :
        event::traits_base<service::States, service::PROPERTY_STATE>
{

};


template <>
struct PropertyTraits2<impl::Service, service::PROPERTY_SUBSTATE> :
        event::traits_base<service::Substates, service::PROPERTY_SUBSTATE>
{
    static constexpr const char* name() { return "Service sub state"; }
};



namespace impl {

struct Service : event::owner_tag
{
    constexpr static const char* name() { return "Generic service"; }
    constexpr static const char* instance() { return ""; }

    struct id
    {
        struct state : event::traits_tag
        {
            typedef service::States value_type;
            static const char* name() { return "Service state"; }
            static constexpr int id() { return service::PROPERTY_STATE; }
        };

        struct substate : event::traits_tag
        {
            typedef service::Substates value_type;
            static const char* name() { return "Service sub state"; }
            static constexpr int id() { return service::PROPERTY_SUBSTATE; }
        };
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

    template <typename TTrait, class TContext>
    void fire_changed3(typename TTrait::value_type v, TContext& context)
    {
        subject_type::notify(event::PropertyChanged<TTrait>{v}, context);
    }

    template <int id_, class T, class TContext>
    void fire_changed4(T v, TContext& context)
    {
        subject_type::notify(event::PropertyChanged<TContext, id_>{&context, v}, context);
    }

    template <typename TTrait, class TContext>
    void fire_changing(
        const typename TTrait::value_type& v_old,
        const typename TTrait::value_type& v, TContext& context)
    {
        subject_type::notify(event::PropertyChanging<TTrait>{v_old, v}, context);
    }

    template <int id, class T, class TOwner>
    void fire_changing4(
        const T& v_old,
        const T& v, TOwner& context)
    {
        subject_type::notify(event::PropertyChanging<TOwner, id>{&context, v_old, v}, context);
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
            base_type::template fire_changing<impl::Service::id::state>(state_.service_, s, context);
            state_.service_ = s;

            base_type::template fire_changed3<experimental::impl::Service::id::state>(s, context);
            //base_type::template fire_changed2<service::PROPERTY_STATE>(s, context);
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

            //base_type::template fire_changed3<experimental::impl::Service::id::substate>(s, context);
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
            base_type::template fire_changing<impl::Service::id::state>(state_.service_, s, context);

            state_.service_ = s;
            state_.service_substate_ = ss;

            base_type::template fire_changed3<experimental::impl::Service::id::state>(s, context);
            //base_type::template fire_changed2<service::PROPERTY_STATE>(s, context);
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
    //using subject_base = typename base_type::subject_type;

protected:
    typedef TImpl impl_type;
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
    {
        base_type::notify(event::Registration{impl().name(), impl().instance()}, *this);
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
