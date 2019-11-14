#include <catch.hpp>

#include <embr/streambuf.h>
#include <embr/observer.h>


using namespace embr::experimental;

template <class TChar>
struct test_streambuf_observer
{
    int counter = 0;
    int counter_sbumpc = 0;

    // Doesn't work as smoothly, and not sure we want it to anyway (look how verbose)
    void on_notify(generic_event_base<TChar, event_type, event_type::sbumpc> e)
    {
        counter_sbumpc++;
    }

    void on_notify(sget_event<TChar> e)
    {
        //counter--;
    }

    void on_notify(generic_event<TChar, event_type::sgetn> e)
    {
        counter++;
    }

/*
    void on_notify(sget_end_exp_event<TChar> e)
    {
        counter++;
    } */
};

TEST_CASE("streambuf test", "[streambuf]")
{
    SECTION("subject_streambuf")
    {
        char buf[128];
        estd::span<char> test((char*)"hello", 5);
        typedef estd::internal::impl::in_span_streambuf<char> streambuf_impl;
        typedef estd::internal::streambuf<streambuf_impl> streambuf_base;

        test_streambuf_observer<char> o;

        auto subject = embr::layer1::make_subject(o);

        typedef subject_streambuf<streambuf_base, decltype(subject)> streambuf_type;
        streambuf_type sb(subject, test);

        int ch = sb.sbumpc();

        int read_back = sb.sgetn(buf, 128);

        REQUIRE(read_back == 4);
        REQUIRE(o.counter == 1);
        REQUIRE(o.counter_sbumpc == 1);

        //sb.sputn("hi2u", 4);
    }
}