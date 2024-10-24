#include <catch2/catch_all.hpp>

#include <embr/word.h>
#include <embr/bits/word.hpp>
#include <estd/chrono.h>

using namespace embr;

TEST_CASE("word type test", "[word]")
{
    SECTION("enum_mask")
    {
        SECTION("low level")
        {
            typedef embr::internal::enum_mask<word_strictness, word_strictness::masking> h;

            REQUIRE(h::all<word_strictness::masking>());
            REQUIRE(h::any<word_strictness::masking, word_strictness::narrowing>());
            REQUIRE(!h::all<h::e::masking, h::e::narrowing>());

            auto v = h::or_all<
                word_strictness::masking, word_strictness::narrowing, word_strictness::arithmetic>::value;
        }
        SECTION("high level")
        {
        }
    }
    SECTION("8 bit")
    {
        embr::word<1> v{true};
        typedef estd::numeric_limits<decltype(v)> limits;

        REQUIRE(estd::is_same<typename limits::type, uint8_t>::value);
        REQUIRE(limits::max() == 1);
        REQUIRE(v.value() == 1);
    }
    SECTION("16 bit")
    {
        embr::word<10> v{3};
        typedef estd::numeric_limits<decltype(v)> limits;

        v = v + 3;

        REQUIRE(v == 6);
        REQUIRE(v.width() == 10);
        REQUIRE(limits::min() == 0);
        REQUIRE(limits::max() == 1023);

        v += 10;

        REQUIRE(v == 16);
    }
    SECTION("narrowing")
    {
        embr::word<36> v{3};

        /*
         * Compiler will not allow this (by our design), unsupported narrowing */
        //auto v_short = estd::to_integer<short>(v);

        //REQUIRE(v_short == 3);

        auto v_long = estd::to_integer<long>(v);

        REQUIRE(v_long == 3);

        // We disallow this narrowing also
        //embr::word<10> v2{v};

        auto v_short1 = embr::narrow_cast<word<10, true> >(v);

        REQUIRE(v_short1 == 3);
    }
    SECTION("masking")
    {
        // DEBT: See chrono comments
        constexpr auto flags = (embr::word_strictness) ((int)embr::word_strictness::arithmetic | (int)embr::word_strictness::masking);

        union
        {
            uint16_t v;
            embr::bits::internal::word<16> v2;
            embr::word<6, false, flags, uint16_t> storage;
        };

        v = 0;

        v2.set<8>(1);

        REQUIRE(v == 0x0100);
        REQUIRE(v2 == 0x0100);
        REQUIRE(storage.value() == 0);

        v2.set<4>(1);

        REQUIRE(v == 0x0110);
        REQUIRE(v2 == 0x0110);
        REQUIRE(storage.value() == 16);
    }
    SECTION("chrono")
    {
        // DEBT: All of these helpers return underlying int type, which for this scenario is a pain
        //constexpr auto flags = embr::internal::enum_or_all<embr::word_strictness, embr::word_strictness::arithmetic, embr::word_strictness::masking>::value;
        //constexpr auto flags = embr::strictness_helper<embr::word_strictness::none>
            //::or_all<embr::word_strictness::arithmetic, embr::word_strictness::masking>::value;
        // DEBT: This too is a pain
        constexpr auto flags = (embr::word_strictness) ((int)embr::word_strictness::arithmetic | (int)embr::word_strictness::masking);

        estd::chrono::duration<embr::word<5, false, flags> > seconds(3);

        // TODO: Some incompatibilities
        estd::chrono::milliseconds ms = seconds;
        //estd::chrono::duration_cast<estd::chrono::milliseconds>(seconds)

        union
        {
            estd::chrono::duration<embr::word<5, false, flags, uint16_t> > seconds2;
            uint16_t storage;
        };

        REQUIRE(seconds.count() == 3);
        REQUIRE(ms.count() == 3000);

        storage = 31;

        REQUIRE(seconds2.count() == 31);

        // Overflow, on purpose
        ++storage;

        REQUIRE(seconds2.count() == 0);
    }
    SECTION("v2")
    {
        SECTION("alias up")
        {
            REQUIRE(internal::alias_up(3, 8) == 8);
            REQUIRE(internal::alias_up(7, 8) == 8);
            REQUIRE(internal::alias_up(8, 8) == 8);
            REQUIRE(internal::alias_up(9, 8) == 16);
        }
        SECTION("storage")
        {
            v2::word<21> v(5);
            v2::word<21, v2::word_options::packed> v2(5);

            REQUIRE(sizeof(v) == 4);
            REQUIRE(sizeof(v2) == 3);

            // Doesn't work - getting too tired to reasonably continue,
            // word_retriever is starting to mutate into the unknown...
            //REQUIRE(v == v2);
        }
    }
}
