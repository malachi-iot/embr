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

    struct id : Service::id
    {

    };
};

}


class DependerService : Service<>
{
    typedef Service<> base_type;

public:
    int counter = 0;
    int counter2 = 0;

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

template <class TSubject>
class DependentService2 : public Service2<::impl::DependentService2, TSubject>
{
    typedef Service2<::impl::DependentService2, TSubject> base_type;

public:
    DependentService2(TSubject&& subject) : base_type(std::move(subject))
    {}
};

TEST_CASE("Services", "[services]")
{
    DependerService depender;

    auto subject = embr::layer1::make_subject(depender);
    auto dependent = make_service<DependentService>(std::move(subject));
    auto dependent2 = make_service<DependentService2>(std::move(subject));

    dependent.start();
    dependent2.start();

    REQUIRE(depender.counter == 3);
    REQUIRE(depender.counter2 == 1);
}