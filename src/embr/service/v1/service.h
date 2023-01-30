#pragma once

#include "../../property/v1/notifier.h"
#include "fwd.h"
#include "enum.h"
#include "event.h"

namespace embr {

inline namespace service { inline namespace v1 {

template <class TService, class enabled = void>
struct ServiceTraits
{
    static const char* name(const TService& s) { return s.name(); }
};

struct SparseService : embr::PropertyContainer,
    ServiceBase
{
    typedef SparseService this_type;

    EMBR_PROPERTIES_SPARSE_BEGIN

        EMBR_PROPERTY_ID_SPARSE(state, States, STATE, "state")
        EMBR_PROPERTY_ID_SPARSE(substate, Substates, SUBSTATE, "substate")

        template <class TConfig>
        using config = traits<TConfig>;

    EMBR_PROPERTIES_SPARSE_END

    template <class TSubject, class TImpl = this_type>
    using runtime = embr::service::v1::host::Service<TImpl, TSubject>;
};

struct Service : embr::PropertyContainer,
        ServiceBase
{
    typedef Service this_type;

    constexpr static const char* name() { return "Generic service"; }
    constexpr static const char* instance() { return ""; }

    // DEBT: Make this private/protected
// protected:
    union
    {
        struct
        {
            States service_: bitsize::state::value;
            Substates service_substate_: bitsize::substate::value;

            States child1 : bitsize::state::value;
            States child2 : bitsize::state::value;
            States child3 : bitsize::state::value;

            // Application specific data in "free" leftover bits
            unsigned user : bitsize::user::value;

        } state_;

        unsigned raw = 0;
    };


    EMBR_PROPERTIES_SPARSE_BEGIN

        EMBR_PROPERTY_ID_EXT(state_.service_, STATE, state, "state")
        EMBR_PROPERTY_ID_EXT(state_.service_substate_, SUBSTATE, substate, "substate")
        EMBR_PROPERTY_ID_EXT(state_.user, USER, user, "app-specific data")

        template <class TConfig>
        using config = traits<TConfig>;

    EMBR_PROPERTIES_SPARSE_END

    template <class TSubject, class TImpl = this_type>
    using runtime = embr::service::v1::host::Service<TImpl, TSubject>;

protected:
    // DEBT: Probably should to a receive command signal switch here instead
    bool restart() { return true; }
    //static constexpr state_result start() { return state_result::started(); }
    bool stop() { return true; }

    // DEBT: Use embr::word here
    unsigned user() const { return state_.user; }

public:
    States state() const { return state_.service_; }
};

namespace host {

template <class TImpl, class TSubject>
class Service : public PropertyHost<TImpl, TSubject>
{
    typedef PropertyHost<TImpl, TSubject> base_type;
    //using subject_base = typename base_type::subject_type;

protected:
    using typename base_type::impl_type;
    using st = v1::Service;
    using typename base_type::context_type;
    using typename base_type::runtime_type;
    using base_type::subject;
    using base_type::runtime;

public:
    using base_type::impl;

protected:
    void state(st::States s)
    {
        base_type::template setter<st::id::state>(s);
    }


    void state(st::Substates s)
    {
        base_type::template setter<st::id::substate>(s);
    }

    void state(st::States s, st::Substates ss)
    {
        if (s != impl().state_.service_)
        {
            base_type::template fire_changing<st::id::state>(impl().state_.service_, s, impl());

            impl().state_.service_ = s;
            impl().state_.service_substate_ = ss;

            base_type::template fire_changed<st::id::state>(s);
            //base_type::template fire_changed2<service::PROPERTY_STATE>(s, context);
        }
    }


    // Convenience method, probably won't get used much
    inline void user(unsigned v)
    {
        base_type::template setter<st::id::user>(v);
    }


    template <class TConfig>
    void configuring(const TConfig* c)
    {
        typedef st::id::config<const TConfig*> traits_type;

        base_type::template fire_changing<traits_type>(nullptr, c, *runtime());
    }


    template <class TConfig>
    void configured(const TConfig* c)
    {
        typedef st::id::config<const TConfig*> traits_type;

        base_type::template fire_changed<traits_type>(c);
    }


    /*
    template <class F, class ...TArgs>
    void start(F&& f, TArgs&&...args)
    {
        state(st::Starting);
        if (f(std::forward<TArgs>(args)...))
        {
            state(st::Started, st::Running);
        }
    } */

    template <class F>
    void stop(F&& f)
    {
        state(st::Stopping);
        f();
        state(st::Stopped, st::Finished);
    }

    void fire_registration()
    {
        // DEBT: Make a special responder which handles stateless subject
        typedef service::v1::ServiceTraits<impl_type> service_traits;

        impl_type& i = impl();

        base_type::notify(event::Registration{service_traits::name(i), i.instance()});
    }
    
public:
    ESTD_CPP_FORWARDING_CTOR(Service)

    Service()
    {
        fire_registration();
    }

    Service(TSubject& subject) : base_type(subject)
    {
        fire_registration();
    }

    Service(TSubject&& subject) : base_type(std::move(subject))
    {
        fire_registration();
    }

    ServiceBase::States state() const { return impl().state_.service_; }

    /*
    ~Service()
    {
        typename impl_type::template responder<TSubject, impl_type> r(impl(), base_type::subject());
        base_type::notify(event::Registration{impl().name(), impl().instance()}, r);
    } */

    template <class ...TArgs>
    void start(TArgs&&...args)
    {
        runtime()->on_starting();
        state(st::Starting);
        //st::state_result r = impl_type::start(std::forward<TArgs>(args)...);
        //if (r.state == st::Started)
        //{
            //context_type context(impl(), subject());

            st::state_result r = runtime()->on_start(std::forward<TArgs>(args)...);

            //r = impl_type::template on_start<TSubject, TImpl>(*runtime());
        //}

        state(r.state, r.substate);
    }

    void restart()
    {
        state(st::Restarting);
        if (impl_type::restart())
        {
            state(st::Started, st::Running);
        }
    }
};

}

template <template <class> class TService, class TSubject>
TService<TSubject> make_service_old(TSubject&& subject)
{
    return TService<TSubject>(std::move(subject));
}

template <class TService, class TSubject>
typename TService::template runtime<TSubject, TService> make_service(TSubject&& subject)
{
    return typename TService::template runtime<TSubject, TService>(std::move(subject));
}

template <class TService, class TSubject>
typename TService::template runtime<TSubject, TService> make_service(TSubject& subject)
{
    return typename TService::template runtime<TSubject, TService>(subject);
}


template <class TImpl, class TSubject>
ServiceSpec<TImpl, TSubject> make_service_spec(TSubject&& subject)
{
    return ServiceSpec<TImpl, TSubject>(std::move(subject));
}

}}

namespace layer0 {

inline namespace service { inline namespace v1 {

// DEBT: Not sure I like this because TImpl itself is not specifically layer0, just the
// observers
template <class TImpl, class ...TObservers>
using service_type = typename TImpl::template runtime<layer0::subject<TObservers...>, TImpl>;

}}

}

}
