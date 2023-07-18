#pragma once

#include "../../property/v1/notifier.h"
#include "fwd.h"
#include "enum.h"
#include "event.h"
#include "macros.h"

#include "internal/service.h"

namespace embr {

inline namespace service { inline namespace v1 {

template <class TService, class enabled = void>
struct ServiceTraits
{
    static const char* name(const TService& s) { return s.name(); }
};

struct Service : embr::property::v1::PropertyContainer,
    ServiceBase
{
private:
    typedef PropertyContainer base_type;
    typedef Service this_type;

public:
    constexpr static const char* name() { return "Generic service"; }

    // DEBT: Make this private/protected
protected:
    union
    {
        // DEBT: Make a 16-bit version of this that only has child1 & child2,
        // right now it's assumed 32-bit
        struct
        {
            States service_: bitsize::state;
            Substates service_substate_: bitsize::substate;

            States child1 : bitsize::state;
            States child2 : bitsize::state;
            States child3 : bitsize::state;

            // Application specific data in "free" leftover bits
            uint32_t user : bitsize::user;

        } state_;

        uint32_t raw = 0;
    };

public:
    EMBR_PROPERTIES_SPARSE_BEGIN

        EMBR_PROPERTY_ID_EXT(state_.service_, STATE, state, "state")
        EMBR_PROPERTY_ID_EXT(state_.service_substate_, SUBSTATE, substate, "substate")
        EMBR_PROPERTY_ID_EXT(state_.user, USER, user, "app-specific data")

        template <class TConfig>
        using config = traits<TConfig>;

    EMBR_PROPERTIES_SPARSE_END

    template <class TSubject, class TImpl = this_type>
    using runtime = embr::service::v1::runtime::Service<TImpl, TSubject>;

protected:
    // DEBT: Probably should receive command signal switch here instead
    bool restart() { return true; }
    //static constexpr state_result start() { return state_result::started(); }
    bool stop() { return true; }

    // DEBT: Use embr::word here
    unsigned user() const { return state_.user; }

public:
    States state() const { return state_.service_; }
    ServiceBase::Substates substate() const { return state_.service_substate_; }
};


namespace host {


namespace internal {

// DEBT: Placement not ideal, but needed since v1::Service must be a complete type
template <class TImpl, class TSubject>
class ServiceBase : public PropertyHost<TImpl, TSubject>
{
    typedef PropertyHost<TImpl, TSubject> base_type;
    using st = v1::Service;

public:
    // DEBT: I don't think we really need this public
    using typename base_type::impl_type;

protected:
    using base_type::impl;

    ESTD_CPP_FORWARDING_CTOR(ServiceBase)

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
        if (s != base_type::state_.service_)
        {
            base_type::template fire_changing<st::id::state>(
                        base_type::state_.service_, s);

            base_type::state_.service_ = s;
            base_type::state_.service_substate_ = ss;

            base_type::template fire_changed<st::id::state>(s);
        }
        else if(ss != base_type::state_.service_substate_)
        {
            state(ss);
        }
    }
};


}

template <class TImpl, class TSubject, class TBase>
class Service : public TBase
{
    typedef TBase base_type;
    //using subject_base = typename base_type::subject_type;

protected:
    using typename base_type::impl_type;
    using st = v1::Service;
#if FEATURE_EMBR_PROPERTY_CONTEXT
    using typename base_type::context_type;
#endif
    using typename base_type::runtime_type;
    using base_type::subject;
    using base_type::runtime;
    using base_type::state;

public:
    using base_type::impl;

protected:
    // Convenience method, probably won't get used much
    inline void user(unsigned v)
    {
        base_type::template setter<st::id::user>(v);
    }


    // DEBT: Mismatch of * vs & for configuring vs configured.  Doing so because
    // we need configuring to be nullable, but & is convenient for the more-used configured
    template <class TConfig>
    void configuring(const TConfig* c)
    {
        typedef st::id::config<const TConfig*> traits_type;

        state(Service::Configuring);

        base_type::template fire_changing<traits_type>(nullptr, c);
    }


    template <class TConfig>
    void configured(const TConfig& c)
    {
        typedef st::id::config<const TConfig&> traits_type;

        base_type::template fire_changed<traits_type>(c);
    }


    void progress(unsigned percent, const char* comment = nullptr)
    {
        base_type::notify(event::Progress{percent, comment});
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

    void fire_registration()
    {
        // DEBT: Make a special responder which handles stateless subject
        typedef service::v1::ServiceTraits<impl_type> service_traits;

        impl_type& i = impl();

        base_type::verify_runtime_integrity(this);
        base_type::notify(event::Registration{service_traits::name(i), i.instance()});
    }
    
public:
    // DEBT: Rework so that we can pass in forwarded parameters down to base service but
    // still fire off registration

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

#if FEATURE_EMBR_PROPERTY_CONTEXT
    // FIX: Even with feature flag on, we may not have visibility into impl() here
    st::States state() const { return impl().state_.service_; }
    st::Substates substate() const { return impl().substate(); }
#else
    st::States state() const { return base_type::state_.service_; }
#endif

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

    void stop()
    {
        state(st::Stopping);
        st::state_result r = runtime()->on_stop();
        state(r.state, r.substate);
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

template <class TService, class TSubject, class ...TArgs>
typename TService::template runtime<TSubject, TService> make_service(TSubject& subject, TArgs&&...args)
{
    return typename TService::template runtime<TSubject, TService>(subject, std::move(args)...);
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
