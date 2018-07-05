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


template <class TNetBufAllocator>
void test(TNetBufAllocator& a)
{
    // FIX: Misbehaves when we really want it to be TNetBufAllocator&
    // runs destructor since we're actually copying to a local TNetBufAllocator
    // rather than a reference, which results in two active TNetBufAllocators
    // - at the moment it doesn't crash, but it will
    test_string<TNetBufAllocator> s(a);

    s += "hello";

    REQUIRE(s == "hello");

    s += " world!";

    REQUIRE(s == "hello world!");
}

TEST_CASE("experimental test", "[experimental]")
{
    SECTION("NetBufAllocator")
    {
        embr::mem::layer1::NetBuf<128> nb;
        NetBufAllocator<char, decltype(nb)& > a(nb);

        test(a);
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

            test(a);
        }
    }
    SECTION("Inline NetBufAllocator + NetBufDynamic")
    {
        NetBufAllocator<char, embr::mem::experimental::NetBufDynamic<> > a;

        // is 32 bytes, likely due to some kind of padding
        //REQUIRE(sizeof(a) == sizeof(int) + sizeof(void*) + sizeof(void*));

        // this works, but destructor seems to be a bit squirrely
        test(a);
    }
}
