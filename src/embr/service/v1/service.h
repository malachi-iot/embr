#pragma once

#include "../../property/v1/notifier.h"
#include "fwd.h"
#include "enum.h"
#include "event.h"

namespace embr {

inline namespace service { inline namespace v1 {

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
            States service_: STATES_MAX_BITSIZE;
            Substates service_substate_: 6;

            States child1: STATES_MAX_BITSIZE;
            States child2: STATES_MAX_BITSIZE;
            States child3: STATES_MAX_BITSIZE;

        } state_;

        unsigned raw = 0;
    };

    EMBR_PROPERTIES_SPARSE_BEGIN

        EMBR_PROPERTY_ID_EXT(state_.service_, PROPERTY_STATE,
                             state, "substate")

        EMBR_PROPERTY_ID_EXT(state_.service_substate_, PROPERTY_SUBSTATE,
                             substate, "substate")

    EMBR_PROPERTIES_SPARSE_END

    template <class TSubject, class TImpl = this_type>
    using runtime = embr::service::v1::host::Service<TImpl, TSubject>;

protected:
    // DEBT: Probably should to a receive command signal switch here instead
    bool restart() { return true; }
    bool start() { return true; }
    bool stop() { return true; }

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
    using base_type::subject;

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
        impl_type& i = impl();
        base_type::notify(event::Registration{i.name(), i.instance()});
    }
    
    // DEBT: Poor naming and general interaction, confusing
    bool on_start()
    {
        return true;
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

    template <class ...TArgs>
    void start(TArgs&&...args)
    {
        state(st::Starting);
        if (impl_type::start(std::forward<TArgs>(args)...))
        {
            context_type context(impl(), subject());

            if(impl_type::template on_start<TSubject, TImpl>(context))
                state(st::Started, st::Running);
        }
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

}
