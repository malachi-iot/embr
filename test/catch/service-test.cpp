#include <catch2/catch.hpp>

#include <embr/exp/service.h>

using namespace embr::experimental;

template <class TSubject>
class DependentService;

class DependerService : Service<>
{
    typedef Service<> base_type;

public:
    int counter = 0;

    //template <class TSubject>
    void on_notify(event::PropertyChanged<service::States> s)//, DependentService<TSubject>&)
    {
        ++counter;
        //FAIL("got here");
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

TEST_CASE("Services", "[services]")
{
    DependerService depender;

    auto subject = embr::layer1::make_subject(depender);
    auto dependent = make_service<DependentService>(std::move(subject));

    dependent.start();

    REQUIRE(depender.counter == 1);
}