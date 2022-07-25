#include <catch.hpp>

#include <embr/netbuf-static.h>
#include <embr/netbuf-dynamic.h>
#include <embr/netbuf-reader.h>
#include <embr/exp/netbuf-alloc.h>

#include <estd/string.h>
#include <estd/string_view.h>

// NOTE: At this time, not yet freertos specific, so test it here in regular GNU area
#include <embr/platform/freertos/exp/transport-retry.h>
#include <embr/streambuf.hpp>
#include <estd/sstream.h>

#include <embr/bits/bits.hpp>

#include "test-data.h"

using namespace embr::experimental;

template <class TTransport, class TRetryPolicyImpl, class TTimer>
std::allocator<typename RetryManager<TTransport, TRetryPolicyImpl, TTimer>::QueuedItem>
        RetryManager<TTransport, TRetryPolicyImpl, TTimer>::stub;

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

    // NOTE: makes no difference
    typedef typename std::remove_reference<TAllocator>::type allocator_type;
};


template <class TAllocator>
void test_alloc(TAllocator& a)
{
    // FIX: Misbehaves when we really want it to be TNetBufAllocator&
    // runs destructor since we're actually copying to a local TNetBufAllocator
    // rather than a reference, which results in two active TNetBufAllocators
    // - at the moment it doesn't crash, but it will
    test_string<TAllocator&> s(a);

    s += "hello";

    // FIX: Although embr netbufs are phased out, this clang incompatibility
    // belies a deeper estd allocator problem
#ifndef __clang__
    REQUIRE(s == "hello");

    s += " world!";

    REQUIRE(s == "hello world!");
#endif
}

TEST_CASE("experimental test", "[experimental]")
{
    SECTION("NetBufAllocator")
    {
        embr::mem::layer1::NetBuf<128> nb;
        NetBufAllocator<char, decltype(nb)& > a(nb);

        test_alloc(a);
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

            test_alloc(a);
        }
    }
    SECTION("Inline NetBufAllocator + NetBufDynamic")
    {
        NetBufAllocator<char, embr::mem::experimental::NetBufDynamic<> > a;

        // is 32 bytes, likely due to some kind of padding
        //REQUIRE(sizeof(a) == sizeof(int) + sizeof(void*) + sizeof(void*));

        // this works, but destructor seems to be a bit squirrely
        test_alloc(a);
    }
    SECTION("Retry v3")
    {
        // stringbufs still a mess, so using span variety
        //typedef estd::experimental::ostringstream<128> ostream_type;
        typedef estd::experimental::ospanstream ostream_type;
        typedef estd::experimental::ispanstream istream_type;

        struct Transport
        {
            typedef int endpoint_type;

            //typedef estd::layer1::stringbuf<128> ostreambuf_type;
            typedef ostream_type::streambuf_type ostreambuf_type;
            typedef istream_type::streambuf_type istreambuf_type;
        };

        struct RetryImpl
        {
            typedef Transport transport_type;
            typedef int key_type;
            typedef unsigned timebase_type;

            struct item_policy_impl_type
            {
                timebase_type get_new_expiry()
                {
                    return 100;
                }
            };

            timebase_type get_relative_expiry(item_policy_impl_type& item)
            {
                return 100;
            }
        };


        struct TimerImpl
        {
            typedef unsigned timebase_type;
            typedef int handle_type;

            handle_type create(timebase_type expiry, void* arg)
            {
                return 0;
            }
        };

        char buf[128];
        estd::span<char> span(buf);

        ostream_type out(span);

        int fake_endpoint = 7;
        auto sb = out.rdbuf();

        embr::experimental::RetryManager<Transport, RetryImpl, TimerImpl> rm;

        // FIX: In its current state, this generates a memory leak since send does a 'new'
        rm.send(fake_endpoint, *sb, 0);
    }
    SECTION("bits")
    {
        using namespace embr;

        SECTION("optimized getter")
        {
            // ambiguous
            //typedef
                //bits::internal::getter<embr::word<10>, bits::little_endian, bits::lsb_to_msb > getter;
            typedef
                bits::internal::getter<uint16_t, bits::little_endian, bits::lsb_to_msb > getter;

            //getter::get2<0, 4>(le_example1);
            auto v = getter::get<4, 8>(le_example1 + 3);

            REQUIRE(v == 0x23);

            SECTION("wrapped in decoder")
            {
                // DEBT: Get a more useful constructor here to copy in external data if so desired
                // DEBT: Use layer2 here
                bits::layer1::decoder<bits::little_endian, 8> d;

                estd::copy_n(le_example1, 4, d.value());

                // grabs middle of LE 32-bit integer, 4 bits in and treated
                // as an 8-bit value
                auto v = d.get<4, 8>(2);

                REQUIRE(v.cvalue() == 0x23);

                auto v2 = d.get<4, 12>(2);

                // This is accurate.  Remember, bitpos is trims off the left END of the word
                // for little endian, thus pushing length further towards the right
                REQUIRE(v2 == 0x123);

                auto v3 = d.get<4, 16>(1);

                REQUIRE(v3.cvalue() == 0x2345);
            }
        }
        SECTION("optimized setter")
        {
            uint8_t raw[8];

            estd::fill_n(raw, 8, 0);

            REQUIRE(embr::bits::experimental::is_subbyte(0, 8) == true);
            REQUIRE(embr::bits::experimental::is_byte_boundary(0, 8) == true);
            REQUIRE(embr::bits::experimental::is_valid(0, 8) == true);

            SECTION("little endian")
            {
                typedef
                    bits::internal::setter<uint16_t, bits::little_endian, bits::lsb_to_msb > setter;

                setter::set<4, 8>(raw, 0x23);

                REQUIRE((int)raw[0] == 0x30);       // lsb_to_msb bitpos=4 means we start 4 bits in
                REQUIRE((int)raw[1] == 0x02);       // lsb_to_msb resume means we start from lsb 0
            }
            SECTION("big endian")
            {
                typedef
                    bits::internal::setter<uint16_t, bits::big_endian, bits::lsb_to_msb > setter;

                SECTION("within-byte operations")
                {
                    setter::set<4, 3>(raw + 1, 2);

                    REQUIRE((int)raw[1] == 0x20);

                    setter::set(bits::descriptor{0, 3}, raw + 1, 2);

                    REQUIRE((int)raw[1] == 0x22);
                }
                SECTION("multi byte operations")
                {
                    //setter::set<4, 8>(raw, 0x23); // disabled as we haven't implemented this specialization yet
                    setter::set<0, 16>(raw, 0x23);
                    setter::set(bits::descriptor{0, 32}, raw + 2, 0x1234);

                    REQUIRE((int)raw[0] == 0);
                    REQUIRE((int)raw[1] == 0x23);

                    REQUIRE((int)raw[4] == 0x12);
                    REQUIRE((int)raw[5] == 0x34);
                }
            }
        }
    }
}
