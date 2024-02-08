#include <catch2/catch.hpp>

#include <embr/streambuf.h>
#include <embr/observer.h>
#include <embr/internal/zip/ostreambuf.h>


using namespace embr::experimental;

template <class TChar>
struct test_streambuf_observer : streambuf::event::observer_base
{
    int counter = 0;
    int counter_sbumpc = 0;

    //typedef streambuf::event::observer_base<TChar> base_type;

    template <type t, phase_type p = phase_type::end>
    using event = streambuf::event::event<TChar, t, p>;

    /*
    // Doesn't work as smoothly, and not sure we want it to anyway (look how verbose)
    void on_notify(generic_event_base<TChar, event_type, event_type::sbumpc> e)
    {
        counter_sbumpc++;
    } */

    /*
    void on_notify(embr::experimental::streambuf::event::event<TChar, type::sbumpc> e)
    {
        counter_sbumpc++;
    } */

    void on_notify(event<type::sbumpc> _e)
    {
        counter_sbumpc++;
    }

    void on_notify(embr::experimental::streambuf::event::sget<TChar> e)
    {
        //counter--;
    }

    void on_notify(event<type::sgetn> e)
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
    SECTION("zip")
    {
        embr::zip::header::EOCD eocd;

        REQUIRE(sizeof(eocd) == 22);
        REQUIRE(sizeof(embr::zip::header::local_file) == 30);

        embr::zip::header::layer1::local_file<64> lf{};

        memcpy(lf.data, "hello", 5);
        lf.length.filename = 5;

        REQUIRE(lf.filename() == "hello");
    }
}