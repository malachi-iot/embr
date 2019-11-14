#include <catch.hpp>

#include <embr/streambuf.h>
#include <embr/observer.h>


using namespace embr::experimental;

template <class TChar>
struct test_streambuf_observer
{
    void on_notify(sget_event<TChar> e)
    {
        
    }
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

        int read_back = sb.sgetn(buf, 128);

        REQUIRE(read_back == 5);
    }
}