#pragma once

#include "property.h"
#include "service-fwd.h"

namespace embr { namespace experimental {

namespace event {

struct Registration
{
    const char* name;
    const char* instance;
};


}


namespace impl {

struct Service : PropertyHost
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
            service::States service_: STATES_MAX_BITSIZE;
            service::Substates service_substate_: 6;

            service::States child1 : STATES_MAX_BITSIZE;
            service::States child2 : STATES_MAX_BITSIZE;
            service::States child3 : STATES_MAX_BITSIZE;

        } state_;

        unsigned raw = 0;
    };

    EMBR_PROPERTY_BEGIN

        EMBR_PROPERTY_ID_EXT(state_.service_, service::PROPERTY_STATE,
                               state, "substate")
        EMBR_PROPERTY_ID_EXT(state_.service_substate_, service::PROPERTY_SUBSTATE,
            substate, "substate")

    EMBR_PROPERTY_END

    // Kinda-sorta creates an alias to original service.  Ultimately we combine
    // this and the original service into one code chunk, likely here
    template <class TSubject, class TImpl>
    struct responder : embr::experimental::Service<
        std::reference_wrapper<TImpl>, std::reference_wrapper<TSubject> >
    {
        typedef embr::experimental::Service<
            std::reference_wrapper<TImpl>, std::reference_wrapper<TSubject> > base_type;

        constexpr responder(TImpl& impl, TSubject& subject) : base_type(impl, subject)
        {}

        // Not needed since all flavors of PropertyHost are already convertible to TImpl&
        //operator TImpl& () { return base_type::impl(); }
    };

    template <class TSubject, class TImpl>
    using runtime = embr::experimental::Service<TSubject, TImpl>;

protected:
    bool start() { return true; }
    bool stop() { return true; }

public:
    service::States state() const { return state_.service_; }
};

}


template <class TImpl, class TSubject>
class Service : public PropertyHost<TImpl, TSubject>
{
    typedef PropertyHost<TImpl, TSubject> base_type;
    //using subject_base = typename base_type::subject_type;

protected:
    using typename base_type::impl_type;
    using base_type::impl;

protected:
    void state(service::States s)
    {
        base_type::template setter<impl::Service::id::state>(s);
    }


    void state(service::Substates s)
    {
        base_type::template setter<impl::Service::id::substate>(s);
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

    void fire_registration()
    {
        // DEBT: Make a special responder which handles stateless subject
        typename impl_type::template responder<TSubject, impl_type> r(impl(), base_type::subject());
        base_type::notify(event::Registration{impl().name(), impl().instance()}, r);
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

    /*
    ~Service()
    {
        typename impl_type::template responder<TSubject, impl_type> r(impl(), base_type::subject());
        base_type::notify(event::Registration{impl().name(), impl().instance()}, r);
    } */

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


template <class TImpl, class TSubject>
ServiceSpec<TImpl, TSubject> make_service_spec(TSubject&& subject)
{
    return ServiceSpec<TImpl, TSubject>(std::move(subject));
}


}}
