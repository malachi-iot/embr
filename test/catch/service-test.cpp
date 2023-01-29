#include <catch2/catch.hpp>

#include <embr/service.h>

#include "property-test.h"

using namespace embr;

constexpr experimental::module_info module{"unit tests"};
constexpr experimental::type_info t1{module, "type1", 1};

class DependentService;

template <class TSubject>
class DependentService2;

class DependentService4;

#define EMBR_PROPERTY_ID_EXP(name, id, desc)  \
    struct property_##name##_type : embr::internal::property::traits_base<this_type, decltype(name), id> \
    {                                     \
        typedef this_type owner_type;                                  \
        static constexpr const char* name() { return desc; }               \
    };

namespace impl {


struct DependentService2 : embr::Service
{
    typedef embr::Service base_type_;
    typedef DependentService2 this_type;

    bool is_shiny = false;
    bool is_happy = false;
    bool is_smiling = false;
    int people = 0;

    enum Properties
    {
        IS_HAPPY = 0,
        IS_SMILING,
        IS_SHINY,
        PEOPLE,
        EVERYWHERE
    };

    static const char* name() { return "DependentService2"; }

    EMBR_PROPERTY_ID_EXP(people, PEOPLE, "people");

    struct id : Service::id
    {
        template <int id, bool = true>
        struct lookup;

        struct is_happy : embr::internal::property::traits_base<this_type, bool, IS_HAPPY>
        {
            static constexpr const char* name() { return "are we happy?"; }

            // Experimental - don't really need to do this, but it's viable.  Maybe useful for updated
            // update-only-if-truly-changed helpers
            static bool get(const DependentService2& d) { return d.is_happy; }
        };

        template <bool dummy>
        struct lookup<IS_HAPPY, dummy> : is_happy {};

        EMBR_PROPERTY_ID_EXT(is_smiling, IS_SMILING, "smiling");
        EMBR_PROPERTY_ID_EXT(is_shiny, IS_SHINY, "shiny");
        EMBR_PROPERTY_ID_EXT(people, PEOPLE, "people");

        /*
        bool everywhere_;
        struct everywhere : event::traits_base<id_type , bool, EVERYWHERE>
        {
            typedef event::traits_base<id_type, bool, EVERYWHERE> base_type;
        }; */

        EMBR_PROPERTY_ID(everywhere, bool, EVERYWHERE, "everywhere?");
        EMBR_PROPERTY_ID(free_floating, float, "all by myself?");
    } fields_;
};

struct DependentService3 : embr::Service
{
    typedef DependentService3 this_type;

    static const char* name() { return "DependentService3"; }

    int value1;
    bool value2;
    const char* value3;

    enum Properties
    {
        VALUE1,
        VALUE2,
        VALUE3
    };

    EMBR_PROPERTIES_SPARSE_BEGIN

    EMBR_PROPERTY_ID_EXT(value1, VALUE1, "desc for value1");
    EMBR_PROPERTY_ID_EXT(value2, VALUE2, "desc for value2");
    EMBR_PROPERTY_ID_EXT(value3, VALUE3, "desc for value3");

    EMBR_PROPERTIES_SPARSE_END
};

}




class DependerService : embr::host::Service<>
{
    typedef Service<> base_type;

public:
    ::impl::DependentService2* ds2 = nullptr;

    int counter = 0;
    int counter2 = 0;
    int battery_level = 0;
    bool battery_alert = false;
    bool is_smiling = false;
    bool is_shiny = false;
    bool shiny_happy_people = false;

    template <class TDummy>
    void register_helper(const TDummy&) {}

    void register_helper(::impl::DependentService2& s)
    {
        ds2 = &s;
    }

    // FIX: Oddly, this works better than Service::context here
    template <class TSubject, class TImpl>
    void on_notify(event::Registration e, embr::Service::runtime<TSubject, TImpl>& context)
    //void on_notify(event::Registration e, ::impl::DependentService2& context)
    {
        printf("Service registered: %s (%s)\n", e.name, e.instance);
        fflush(stdout);
        // FIX: reference_wrapper doesn't appear to auto unwrap as expected,
        // have to nudge it along.  Even with underlying conversion operator helping us,
        // it doesn't auto convert in this case
        //embr::experimental::unwrap_t<TImpl>& impl = context;
        //TImpl& impl = context;
        register_helper(context.impl());
        //register_helper(context);
        //register_helper(impl);
        //ds2 = &context;
    }

    //template <class TSubject>
    void on_notify(event::PropertyChanged<embr::Service::States> e)//, DependentService<TSubject>&)
    {
        if(e.owner == ds2)  // FIX: This isn't working, but given all the impl() and conversion going on, that's not surprising
            impl().state_.child1 = e.value;

        ++counter;
        //FAIL("got here");
    }

    // TODO: This "legacy" way is broken now, but for limited scenarios we would still like
    // this approach to function
    //void on_notify(event::PropertyChanged<service::Substates, service::PROPERTY_SUBSTATE> e,
    void on_notify(event::PropertyChanged<embr::Service, embr::Service::SUBSTATE> e,
        ::impl::DependentService2& c)
    {
        if(e.value == Service::Starting)
        {
            ++counter;
        }
    }

    /*
    void on_notify(event::PropertyChanged<service::States> s, impl::DependentService2& c)
    {
        ++counter;
        //FAIL("got here");
    } */

    void on_notify(event::PropertyChanged<Service::id::state> s, ::impl::DependentService2& c)
    {
        ++counter;
        ++counter2;
        printf("'%s': id=%d, value=%d\n", s.name(), s.id(), s.value);
        fflush(stdout);
    }

    void on_notify(event::PropertyChanged<::impl::DependentService2::id::is_happy> e)
    {
        ++counter2;
        REQUIRE(e.value == true);
    }

    void on_notify(event::PropertyChanged<::impl::DependentService2::id::is_smiling> e)
    {
        is_smiling = e.value;
    }

    void on_notify(event::PropertyChanged<
        ::impl::DependentService2,
        ::impl::DependentService2::IS_SHINY> e)
    {
        is_shiny = e.value;
    }


    void on_notify(event::PropertyChanged<::impl::DependentService2> e)
    {
        switch(e.id())
        {
            case ::impl::DependentService2::PEOPLE:
                if(e.owner->people == 10 && is_shiny)
                    shiny_happy_people = true;
                break;

            default: break;
        }
    }

    void on_notify(event::PropertyChanged<Filter1Base::id::battery_level> e)
    {
        battery_level = e.value;
    }

    void on_notify(event::PropertyChanged<Filter1Base, Filter1Base::BATTERY_ALERT> e)
    {
        battery_alert = e.value;
    }
};



class DependentService : public Service
{
    typedef DependentService this_type;

public:

    template <class TSubject, class TImpl = this_type>
    struct runtime : Service::runtime<TSubject, TImpl>
    {
        typedef Service::runtime<TSubject, TImpl> base_type;

        ESTD_CPP_FORWARDING_CTOR(runtime)
    };
};


// NOTE: Although this flavor works, it lacking a 'runtime' means that context will never
// pass back in the full wrapped DependentService2.
template <class TSubject>
class DependentService2 : public host::Service<::impl::DependentService2, TSubject>
{
    typedef host::Service<::impl::DependentService2, TSubject> base_type;
    using typename base_type::impl_type;

    impl_type& impl() { return base_type::impl(); }

public:
    DependentService2() = default;
    DependentService2(TSubject&& subject) : base_type(std::move(subject))
    {}

    void happy(bool v)
    {
        if(base_type::impl().is_happy != v)
        {
            base_type::impl().is_happy = v;
            //typedef ::impl::DependentService2::id::is_happy traits_type;
            typedef typename base_type::impl_type::id::is_happy traits_type;
            event::PropertyChanged<traits_type> p{nullptr, v};
            REQUIRE(traits_type::id() == 0);
            //base_type::template fire_changed3<base_type::impl_type::id::is_happy>(v, *this);
            base_type::template fire_changed<traits_type>(v);
        }
    }

    EMBR_PROPERTY_ALIAS(is_smiling, smiling)

    void people(int v)
    {
        if(impl().people != v)
        {
            impl().people = v;
            base_type::template fire_changed<impl_type, impl_type::PEOPLE>(v);
        }
    }


    void shiny(bool v)
    {
        base_type::template setter<impl_type::IS_SHINY>(v);
    }

    bool shiny() const { return impl().is_shiny; }
};




namespace embr { inline namespace service { inline namespace v1 {
template <class TSubject>
class ServiceSpec<::impl::DependentService3, TSubject> :
        public host::Service<::impl::DependentService3, TSubject>
{
    typedef ::impl::DependentService3 impl_type;
    typedef host::Service<impl_type, TSubject> base_type;

public:
    ESTD_CPP_FORWARDING_CTOR(ServiceSpec)

    EMBR_PROPERTY(value1)
    EMBR_PROPERTY(value2)
    EMBR_PROPERTY(value3)
};

}}}

class DependentService4 : public embr::Service
{
    typedef DependentService4 this_type;

protected:
    int value1 = 0;
    const char* value2;

    int do_private_stuff() const
    {
        INFO("DependentService4: doing private stuff")
        return value1 + 1;
    }


    state_result on_start(const char* value2)
    {
        this->value2 = value2;
        return state_result::started();
    }

    /*
     * DEBT: So far, no RAII for services
    DependentService4(const char* special_init)
    {

    } */

public:
    static const char* name() { return "DependentService4"; }

    enum Properties
    {
        VALUE1,
        VALUE2
    };

    EMBR_PROPERTIES_BEGIN

        EMBR_PROPERTY_ID_EXT(value1, VALUE1, "value1 desc")
        EMBR_PROPERTY_ID_EXT(value2, VALUE2, "value1 desc")
        EMBR_PROPERTY_ID(value3, float, "free floater")
        EMBR_PROPERTY_ID(value4, float, "free floater #2")

    EMBR_PROPERTIES_END

    template <class TSubject, class TImpl = this_type>
    struct runtime : Service::runtime<TSubject, TImpl>
    {
        typedef Service::runtime<TSubject, TImpl> base_type;
        using typename base_type::impl_type;
        using base_type::impl;

        ESTD_CPP_FORWARDING_CTOR(runtime)

        void proxy();

    private:
        EMBR_PROPERTY(value1)
        EMBR_PROPERTY(value2)

    public:
        EMBR_PROPERTY(value3)
    };
};

// increments private value1 by 1
template <class TSubject, class TImpl>
void DependentService4::runtime<TSubject, TImpl>::proxy()
{
    int v = impl().do_private_stuff();

    value1(v);
}

class Filter1 : Filter1Base
{
public:
    template <class TSubject>
    class runtime : public PropertyNotifier<TSubject>
    {
        typedef PropertyNotifier<TSubject> base_type;

    public:
        void on_notify(event::PropertyChanged<DependentService4::id::value1> e)
        {
            auto context = this_type{};  // DEBT
            base_type::template fire_changed<id::battery_level>(e.value * 10, context);
            base_type::template fire_changed<id::battery_alert>(e.value > 0, context);
        }
    };
};

static DependerService d;

TEST_CASE("Services", "[services]")
{
    SECTION("event conversion")
    {
        event::PropertyChanged<
            embr::Service, Service::STATE>
            e1(nullptr, Service::Stopped);

        // DEBT: Something looks suspicious here
        event::PropertyChanged<
            ::impl::DependentService2, Service::STATE>
            e2(nullptr, true);

        event::PropertyChanged<embr::Service::id::state> e3(nullptr, Service::Stopped);
        event::PropertyChanged<embr::Service::States> e4(e3);

        REQUIRE(e4.id() == embr::Service::STATE);
        REQUIRE(e4.value == e3.value);

        typedef PropertyTraits<::impl::DependentService2, ::impl::DependentService2::PEOPLE> traits1;

        REQUIRE(traits1::id() == ::impl::DependentService2::PEOPLE);

        REQUIRE(e2.value == true);
    }

    //DependerService depender;
    DependerService& depender = d;

    typedef estd::integral_constant<DependerService*, &d> _type_;
    //typedef embr::internal::static_wrapper<DependerService, &d> _type_;

    _type_ v;
    //DependerService& _d = (_type_());
    //DependerService* __d = &_d;
    DependerService* __d = _type_::value;
    DependerService* ___d = &d;

    typedef embr::layer0::subject<_type_> subject_type;
    auto subject = subject_type();
    //auto subject = embr::layer1::make_subject(depender);
    auto& s = subject;

    typedef Filter1::runtime<subject_type> filter1_type;

    // FIX: These should be using reference, not rvalue
    auto dependent = make_service<DependentService>(subject);
    /*
    auto dependent2 = make_service<DependentService2>(std::move(subject));
    auto dependent3 = make_service_spec<::impl::DependentService3>(std::move(subject));
    //auto dependent4 = make_service<DependentService4::service>(std::move(subject));
    auto dependent4 = DependentService4::service<decltype(subject)>(s); */
    //DependentService<subject_type> dependent;
    DependentService2<subject_type> dependent2;
    service::ServiceSpec<::impl::DependentService3, subject_type> dependent3;
    layer0::service_type<DependentService4, _type_, filter1_type> dependent4;


    dependent.start();
    dependent2.start();
    dependent3.start();
    dependent4.start("value2 initialized");

    dependent2.shiny(true);
    dependent2.happy(true);
    dependent2.smiling(true);
    dependent2.people(10);

    dependent3.value1(7);

    dependent4.proxy();
    dependent4.value3(12);

    REQUIRE(depender.counter == 5);     // FIX: This is broken now
    REQUIRE(depender.counter2 == 2);
    REQUIRE(depender.is_smiling);
    REQUIRE(depender.shiny_happy_people == true);
    REQUIRE(depender.ds2 != nullptr);
    REQUIRE(depender.battery_level == 10);
    REQUIRE(depender.battery_alert > 0);

    REQUIRE(dependent4.value3() == 12);

    REQUIRE(&dependent2 == dependent2.debug_runtime());
    REQUIRE(&dependent4 == dependent4.debug_runtime());

}
