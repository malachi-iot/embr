#include <catch2/catch.hpp>

#include <embr/exp/service.h>
#include <embr/exp/service.hpp>

using namespace embr::experimental;

template <class TSubject>
class DependentService;

template <class TSubject>
class DependentService2;

#define EMBR_PROPERTY_ID(name, id, desc)  \
    struct property_##name##_type : event::traits_base<decltype(name), id> \
    {                                     \
        typedef this_type owner_type;                                  \
        static constexpr const char* name() { return desc; }               \
    };

namespace impl {


struct DependentService2 : embr::experimental::impl::Service
{
    typedef embr::experimental::impl::Service base_type_;
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
        PEOPLE
    };

    static const char* name() { return "DeendentService2"; }

    EMBR_PROPERTY_ID(people, PEOPLE, "people");

    struct id : Service::id
    {
        template <int id, bool = true>
        struct lookup;

        struct is_happy : event::traits_base<bool, IS_HAPPY>
        {
            static constexpr const char* name() { return "are we happy?"; }

            // Experimental - don't really need to do this, but it's viable.  Maybe useful for updated
            // update-only-if-truly-changed helpers
            static bool get(const DependentService2& d) { return d.is_happy; }
        };

        //typedef event::traits_base<bool, IS_SMILING> is_smiling;
        EMBR_PROPERTY_ID2(is_smiling, IS_SMILING, "smiling");
        typedef event::traits_base<bool, IS_SHINY> is_shiny;
        //typedef event::traits_base<int, PEOPLE> people;
        EMBR_PROPERTY_ID2(people, PEOPLE, "people");

        template <bool dummy>
        struct lookup<PEOPLE, dummy> : people {};
    };

    template <class TSubject, class TImpl = this_type>
    struct responder : base_type_::responder<TSubject, TImpl>
    {
        typedef base_type_::responder<TSubject, TImpl> base_type;

        ESTD_CPP_FORWARDING_CTOR(responder)
    };
};

#define EMBR_PROPERTY_BEGIN \
struct id : event::lookup_tag  \
{\
    template <int id_, bool = true> struct lookup;

#define EMBR_PROPERTY_END };

#define EMBR_PROPERTY_ID3(name_, id_, desc_) \
    EMBR_PROPERTY_ID2(name_, id_, desc_)     \
    template <bool dummy> struct lookup<id_, dummy> : name_ {};

struct DependentService3 : embr::experimental::impl::Service
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

    EMBR_PROPERTY_BEGIN

    // NOTE: I think we could actually define the variables at the same
    // time with these macros...

    EMBR_PROPERTY_ID3(value1, VALUE1, "desc for value1");
    EMBR_PROPERTY_ID3(value2, VALUE2, "desc for value2");
    EMBR_PROPERTY_ID3(value3, VALUE3, "desc for value3");

    EMBR_PROPERTY_END
};

}


#define EMBR_PROPERTY_DECLARATION(owner, name_, id, desc)  \
template <> struct PropertyTraits2<owner, owner::id> : \
    EMBR_PROPERTY_TRAITS_BASE(owner, name_, owner::id, desc)

#define EMBR_PROPERTY_DECLARATION2(owner, name_)  \
template <> \
struct PropertyTraits2<owner, owner::id::name_::id()> : \
    owner::id::name_ {};

namespace embr { namespace experimental {

template <>
struct PropertyTraits2<::impl::DependentService2, ::impl::DependentService2::IS_HAPPY> :
        event::traits_base<bool, ::impl::DependentService2::IS_HAPPY>
{
    static constexpr const char* name() { return "are we happy?"; }
};

/*template <>
struct PropertyTraits2<::impl::DependentService2, ::impl::DependentService2::IS_SHINY> :
        ::impl::DependentService2::id::is_shiny
{

}; */

EMBR_PROPERTY_DECLARATION(::impl::DependentService2, is_shiny, IS_SHINY, "shiny");
//EMBR_PROPERTY_DECLARATION(::impl::DependentService2, people, PEOPLE, "people");
EMBR_PROPERTY_DECLARATION2(::impl::DependentService2, is_smiling)
EMBR_PROPERTY_DECLARATION2(::impl::DependentService2, people)

}}



class DependerService : Service<>
{
    typedef Service<> base_type;

public:
    ::impl::DependentService2* ds2 = nullptr;

    int counter = 0;
    int counter2 = 0;
    bool is_smiling = false;
    bool is_shiny = false;
    bool shiny_happy_people = false;

    template <class TDummy>
    void register_helper(const TDummy&) {}

    void register_helper(::impl::DependentService2& s)
    {
        ds2 = &s;
    }

    // FIX: Doesn't reach here, perhaps due to reference wrapping?
    template <class TSubject, class TImpl>
    void on_notify(event::Registration e, embr::experimental::impl::Service::responder<TSubject, TImpl>& context)
    //void on_notify(event::Registration e, ::impl::DependentService2& context)
    {
        printf("Service registered: %s (%s)\n", e.name, e.instance);
        fflush(stdout);
        TImpl& impl = context;
        register_helper(impl);
        //ds2 = &context;
    }

    //template <class TSubject>
    void on_notify(event::PropertyChanged<service::States> s)//, DependentService<TSubject>&)
    {
        ++counter;
        //FAIL("got here");
    }

    // TODO: This "legacy" way is broken now, but for limited scenarios we would still like
    // this approach to function
    //void on_notify(event::PropertyChanged<service::Substates, service::PROPERTY_SUBSTATE> e,
    void on_notify(event::PropertyChanged<embr::experimental::impl::Service, service::PROPERTY_SUBSTATE> e,
        ::impl::DependentService2& c)
    {
        if(e.value == service::Starting)
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

    void on_notify(event::PropertyChanged<embr::experimental::impl::Service::id::state> s, ::impl::DependentService2& c)
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
};


template <class TSubject>
class DependentService : Service<embr::experimental::impl::Service, TSubject>
{
    typedef Service<embr::experimental::impl::Service, TSubject> base_type;

public:
    DependentService(TSubject&& subject) : base_type(std::move(subject))
    {}

    void start()
    {
        base_type::start([]
        {
            return true;
        });
    }
};



#define GETTER_HELPER(field_name) \
    (base_type::impl(). field_name)

#define SETTER_HELPER(field_name) \
    if(base_type::impl(). field_name != v) \
{                                 \
    typedef typename base_type::impl_type::id:: field_name traits_type; \
    base_type::template fire_changing<traits_type>(base_type::impl(). field_name, v, *this);   \
    base_type::impl(). field_name = v;     \
    base_type::template fire_changed3<traits_type>(v, *this);   \
}

#define PROPERTY_HELPER(field_name) \
    typedef typename base_type::impl_type::id:: field_name ::value_type field_name##_value_type    \
    field_name##_value_type field_name() const                                                     \
{ return GETTER_HELPER(field_name);}       \
    void field_name(field_name##_value_type v)                                                     \
    { SETTER_HELPER(field_name); }

template <class TSubject>
class DependentService2 : public Service<::impl::DependentService2, TSubject>
{
    typedef Service<::impl::DependentService2, TSubject> base_type;
    using typename base_type::impl_type;

    impl_type& impl() { return base_type::impl(); }

public:
    DependentService2(TSubject&& subject) : base_type(std::move(subject))
    {}

    void happy(bool v)
    {
        if(base_type::impl().is_happy != v)
        {
            base_type::impl().is_happy = v;
            //typedef ::impl::DependentService2::id::is_happy traits_type;
            typedef typename base_type::impl_type::id::is_happy traits_type;
            event::PropertyChanged<traits_type> p{v};
            REQUIRE(traits_type::id() == 0);
            //base_type::template fire_changed3<base_type::impl_type::id::is_happy>(v, *this);
            base_type::template fire_changed3<traits_type>(v, *this);
        }
    }

    void smiling(bool v)
    {
        SETTER_HELPER(is_smiling)
    }

    bool smiling() const
    { return GETTER_HELPER(is_smiling); }

    void people(int v)
    {
        if(impl().people != v)
        {
            impl().people = v;
            //typedef PropertyTraits2<impl_type, impl_type::PEOPLE> traits_type;
            event::PropertyChanged<impl_type> e(&impl(), v);
            base_type::template fire_changed4<impl_type::PEOPLE, impl_type>(v, impl());
        }
    }

    void shiny(bool v)
    {
        base_type::template setter<impl_type::IS_SHINY>(v);
    }

    bool shiny() const { return impl().is_shiny; }
};

#define _GETTER_HELPER2(type_, name_)   \
    type_ name_() const { return base_type::impl().name_; }
#define _SETTER_HELPER2(type_, name_)   \
    void name_(const type_& v)  \
{ base_type::template setter<impl_type::id::name_>(v, base_type::impl()); }

#define GETTER_HELPER2(name_) \
    _GETTER_HELPER2(typename impl_type::id::name_::value_type, name_)

#define _PROPERTY_HELPER2(type_, name_)     \
    type_ name_() const { return base_type::impl().name_; } \
    void name_(const type_& v)  \
{ base_type::template setter3<impl_type::id::name_::id(), impl_type>(v, base_type::impl()); }

#define PROPERTY_HELPER2(name_) \
        _PROPERTY_HELPER2(typename impl_type::id::name_::value_type, name_)


namespace embr { namespace experimental {
template <class TSubject>
class ServiceSpec<::impl::DependentService3, TSubject> :
        public Service<::impl::DependentService3, TSubject>
{
    typedef ::impl::DependentService3 impl_type;
    typedef Service<impl_type, TSubject> base_type;

public:
    ESTD_CPP_FORWARDING_CTOR(ServiceSpec)

    PROPERTY_HELPER2(value1)
    PROPERTY_HELPER2(value2)
    PROPERTY_HELPER2(value3)
};

}}

class DependentService4 : public embr::experimental::impl::Service
{
    typedef DependentService4 this_type;

private:
    int value1 = 0;
    const char* value2;

    int do_private_stuff() const
    {
        INFO("DependentService4: doing private stuff")
        return value1 + 1;
    }

public:
    enum Properties
    {
        VALUE1,
        VALUE2
    };

    EMBR_PROPERTY_BEGIN

        EMBR_PROPERTY_ID3(value1, VALUE1, "value1 desc")
        EMBR_PROPERTY_ID3(value2, VALUE2, "value1 desc")

    EMBR_PROPERTY_END

    template <class TSubject, class TImpl = this_type>
    struct service : embr::experimental::Service<TImpl, TSubject>
    {
        typedef embr::experimental::Service<TImpl, TSubject> base_type;
        using typename base_type::impl_type;
        using base_type::impl;

        ESTD_CPP_FORWARDING_CTOR(service)

        void proxy();

    private:
        PROPERTY_HELPER2(value1)
        PROPERTY_HELPER2(value2)
    };
};


template <class TSubject, class TImpl>
void DependentService4::service<TSubject, TImpl>::proxy()
{
    int v = impl().do_private_stuff();

    value1(v);
}

TEST_CASE("Services", "[services]")
{
    DependerService depender;

    auto subject = embr::layer1::make_subject(depender);

    // FIX: These should be using reference, not rvalue
    auto dependent = make_service<DependentService>(std::move(subject));
    auto dependent2 = make_service<DependentService2>(std::move(subject));
    auto dependent3 = make_service_spec<::impl::DependentService3>(std::move(subject));
    //auto dependent4 = make_service<DependentService4::service>(std::move(subject));
    auto dependent4 = DependentService4::service<decltype(subject)>(std::move(subject));

    dependent.start();
    dependent2.start();
    dependent3.start();
    dependent4.start();

    dependent2.shiny(true);
    dependent2.happy(true);
    dependent2.smiling(true);
    dependent2.people(10);

    //dependent3.value1(7);

    dependent4.proxy();

    REQUIRE(depender.counter == 5);
    REQUIRE(depender.counter2 == 2);
    REQUIRE(depender.is_smiling);
    REQUIRE(depender.shiny_happy_people == true);
    REQUIRE(depender.ds2 != nullptr);

    event::PropertyChanged<
            embr::experimental::impl::Service, service::PROPERTY_STATE>
            e1(nullptr, service::Stopped);

    event::PropertyChanged<
            ::impl::DependentService2, service::PROPERTY_STATE>
            e2(nullptr, true);

    event::PropertyChanged<embr::experimental::impl::Service::id::state> e3(service::Stopped);

    typedef PropertyTraits3<::impl::DependentService2, ::impl::DependentService2::PEOPLE> traits1;

    REQUIRE(traits1::id() == ::impl::DependentService2::PEOPLE);

    REQUIRE(e2.value == true);
}
