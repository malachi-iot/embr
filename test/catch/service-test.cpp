#include <catch2/catch.hpp>

#include <embr/exp/service.h>

using namespace embr::experimental;

template <class TSubject>
class DependentService;

class DependerService : PropertyNotifier<>
{
    typedef PropertyNotifier<> base_type;

public:
    //template <class TSubject>
    void on_notify(event::PropertyChanged)
    {
        FAIL("got here");
    }
};


template <class TSubject>
class DependentService : PropertyNotifier<TSubject>
{
    typedef PropertyNotifier<TSubject> base_type;

public:
    DependentService(TSubject&& subject) : base_type(std::move(subject))
    {}

    void start()
    {
        base_type::state(service::Starting);
    }
};

TEST_CASE("Services", "[services]")
{
    DependerService depender;

    auto subject = embr::layer1::make_subject(depender);

    DependentService<decltype(subject)> dependent(std::move(subject));

    dependent.start();
}