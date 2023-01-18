#include <catch2/catch.hpp>

#include <embr/exp/service.h>

using namespace embr::experimental;

template <class TSubject>
class DependentService;

template <class TSubject>
class DependentService2;

namespace impl {


struct DependentService2 : embr::experimental::impl::Service
{
    bool is_happy = false;
    bool is_smiling = false;

    enum Properties
    {
        IS_HAPPY = 0,
        IS_SMILING = 1
    };

    struct id : Service::id
    {
        struct is_happy : event::traits_base<bool, IS_HAPPY>
        {
            static constexpr const char* name() { return "are we happy?"; }

            // Experimental - don't really need to do this, but it's viable.  Maybe useful for updated
            // update-only-if-truly-changed helpers
            static bool get(const DependentService2& d) { return d.is_happy; }
        };

        typedef event::traits_base<bool, IS_SMILING> is_smiling;
    };
};

}


class DependerService : Service<>
{
    typedef Service<> base_type;

public:
    int counter = 0;
    int counter2 = 0;
    bool is_smiling = false;

    //template <class TSubject>
    void on_notify(event::PropertyChanged<service::States> s)//, DependentService<TSubject>&)
    {
        ++counter;
        //FAIL("got here");
    }

    void on_notify(event::PropertyChanged<service::Substates, service::PROPERTY_SUBSTATE> s,
        ::impl::DependentService2& c)
    {
        if(s.value == service::Starting)
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
};


template <class TSubject>
class DependentService : Service<TSubject>
{
    typedef Service<TSubject> base_type;

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
    base_type::impl(). field_name = v;     \
    typedef typename base_type::impl_type::id:: field_name traits_type; \
    base_type::template fire_changed3<traits_type>(v, *this);   \
}

#define PROPERTY_HELPER(field_name) \
    typedef typename base_type::impl_type::id:: field_name ::value_type field_name##_value_type    \
    field_name##_value_type field_name() const                                                     \
{ return GETTER_HELPER(field_name);}       \
    void field_name(field_name##_value_type v)                                                     \
    { SETTER_HELPER(field_name); }

template <class TSubject>
class DependentService2 : public Service2<::impl::DependentService2, TSubject>
{
    typedef Service2<::impl::DependentService2, TSubject> base_type;

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
};

TEST_CASE("Services", "[services]")
{
    DependerService depender;

    auto subject = embr::layer1::make_subject(depender);
    auto dependent = make_service<DependentService>(std::move(subject));
    auto dependent2 = make_service<DependentService2>(std::move(subject));

    dependent.start();
    dependent2.start();

    dependent2.happy(true);
    dependent2.smiling(true);

    REQUIRE(depender.counter == 3);
    REQUIRE(depender.counter2 == 2);
    REQUIRE(depender.is_smiling);
}