#include <catch.hpp>

#include <embr/netbuf-static.h>
#include <embr/netbuf-dynamic.h>
#include <embr/netbuf-reader.h>
#include <embr/exp/netbuf-alloc.h>

#include <estd/string.h>
#include <estd/string_view.h>

using namespace embr::experimental;

template <class TAllocator>
class test_string : public estd::basic_string<
        char,
        std::char_traits<char>,
        TAllocator>
{
    typedef estd::basic_string<
            char,
            std::char_traits<char>,
            TAllocator> base;

public:
    test_string(TAllocator& a) : base(a) {}
};

TEST_CASE("experimental test", "[experimental]")
{
    SECTION("NetBufAllocator")
    {
        embr::mem::layer1::NetBuf<128> nb;
        NetBufAllocator<char, decltype(nb)& > a(nb);
        test_string<decltype(a)> s(a);

        s += "hello";

        REQUIRE(s == "hello");
    }
    SECTION("NetBufDynamic")
    {
        embr::mem::experimental::NetBufDynamic<> nb;

        REQUIRE(nb.size() == 0);
        REQUIRE(nb.data() == NULLPTR);
        REQUIRE(nb.total_size() == 0);

        SECTION("Coupled with NetBufAllocator")
        {
            // working well-ish but have yet to test actual chaining
            // it's clear naming is a little confusing here, NetBufAllocator and NetBufDynamic
            NetBufAllocator<char, decltype(nb)& > a(nb);

            test_string<decltype(a)> s(a);

            s += "hello";

            REQUIRE(s == "hello");

            s += " world!";

            REQUIRE(s == "hello world!");
        }
    }
}
