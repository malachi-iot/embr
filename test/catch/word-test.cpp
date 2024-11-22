#include <catch2/catch_all.hpp>

#include <estd/chrono.h>

// DEBT: Seems very much like this should get auto included
#include <estd/internal/units/operators.hpp>

#include <embr/word.h>
#include <embr/bits/word.hpp>

#include <embr/internal/word/v2/word.h>
#include <embr/internal/word/v2/operators.h>

// For testing on new 'v2' word.  Once v2 word settles down, move word-units test out to units area
#include <embr/units/meters.h>

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
            SECTION("support")
            {
                uint8_t v[4] { 1, 2, 3, 4 };

                SECTION("fancy array init")
                {
                    uint8_t out[4];
                    new (out) embr::internal::set_elements<4, uint8_t>{v};

                    REQUIRE(memcmp(out, v, 4) == 0);
                }
                SECTION("array")
                {
                    embr::internal::fill_zero_n(v, 4);
                }
                SECTION("noloop_reverse_copy")
                {
                    uint8_t out[4];
                    embr::internal::noloop_reverse_copy<4>(v, out);

                    REQUIRE(out[0] == 4);
                    REQUIRE(out[1] == 3);
                    REQUIRE(out[2] == 2);
                    REQUIRE(out[3] == 1);
                }
            }
            SECTION("native")
            {
                v2::word<21> v(5);
                v2::word<21, v2::word_options::packed> v2(5);
                v2::word<48, v2::word_options::packed> v3(5);
                v2::word<65, v2::word_options::packed> v4(5);
                constexpr static v2::word<7> c1{5};

                uint64_t _v3 = v3.value();
                uint32_t _v4 = v4.value();

                REQUIRE(sizeof(v) == 4);
                REQUIRE(sizeof(v2) == 3);
                REQUIRE(sizeof(v3) == 6);
                REQUIRE(sizeof(v4) == 9);

                // Doesn't work - getting too tired to reasonably continue,
                // word_retriever is starting to mutate into the unknown...
                REQUIRE(v == v2);
                REQUIRE(v3 == v2);
                REQUIRE(v4 == v);
                REQUIRE(v4 == c1);
            }
            SECTION("big endian")
            {
                v2::word<21, v2::word_options::big_endian> v(5);
                v2::word<21, v2::word_options::big_endian | v2::word_options::packed> v2(5);
                v2::word<11, v2::word_options::big_endian> v3(5);
                v2::word<32, v2::word_options::big_endian> v4(5);

                REQUIRE(sizeof(v) == 4);
                REQUIRE(sizeof(v2) == 3);

#if __LITTLE_ENDIAN__
                REQUIRE(v.v_ == 0x05000000);
#endif
                REQUIRE(v2.raw_[2] == 0x05);
                REQUIRE(v == v3);
                REQUIRE(v == v4);
            }
            SECTION("little endian")
            {
                v2::word<21, v2::word_options::little_endian> v(5);
                v2::word<21, v2::word_options::little_endian | v2::word_options::packed> v2(5);

                REQUIRE(sizeof(v) == 4);
                REQUIRE(sizeof(v2) == 3);

                REQUIRE(v2.raw_[0] == 0x05);
#if __LITTLE_ENDIAN__
                REQUIRE(v == 5);
#endif
            }
            SECTION("raw byte")
            {
                //internal::word_v2_base<24, v2::word_options::packed> v(float{}, 0, 1, 2);
                static constexpr internal::word_v2_base<24,
                    v2::word_options::packed | v2::word_options::big_endian> c1({ 0, 1, 2 });

                // FIX: This one is unhappy
                //unsigned v1 = c1.value();
                //bool b1 = v1 == c1;

                REQUIRE(c1 == 0x102);
            }
            SECTION("detail::v2")
            {
                SECTION("raw 15 bit, base")
                {
                    using traits = internal::word_traits<15, v2::word_options::raw>;

                    using type = detail::v2::word_base<traits>;

                    type w{};

                    REQUIRE(w.value()[0] == 0);
                }
                SECTION("natural 15 bit, base")
                {
                    using traits = internal::word_traits<15, v2::word_options::none>;

                    using type = detail::v2::word_base<traits>;

                    type w{};

                    REQUIRE(w.value() == 0);
                }
                SECTION("raw 15 bit")
                {
                    using traits = internal::word_traits<15, v2::word_options::raw>;

                    using type = detail::v2::word<traits>;

                    type w{5};

                    REQUIRE(w.value() == 5);
                }
            }
        }
        SECTION("implicit")
        {
            SECTION("equals")
            {
                v2::word<21, v2::word_options::implicit> v(5);

                REQUIRE(v == 5);
            }
            SECTION("conversion")
            {
                v2::word<21> v(5);
                v2::word<21, v2::word_options::implicit> v2(v);
                v2::word<21> v3(v2);

                REQUIRE(v2 == 5);
                REQUIRE(v2 == v3);
            }
            SECTION("cast")
            {
                constexpr v2::word_options o{}, o2{}, o3{v2::word_options::implicit};

                bool b1 = (o & ~v2::word_options::implicit) == (o2 & ~v2::word_options::implicit);

                REQUIRE(b1);

                static_assert(internal::is_castable<o, o2>::value, "default values should always match");
                static_assert(internal::is_castable<o, o3>::value, "implicit shouldn't disqualify castable");
                static_assert(internal::is_castable<o, v2::word_options::implicit>::value, "implicit shouldn't disqualify castable");

                static_assert(internal::is_castable<o, v2::word_options::packed>::value == false,
                    "both or neither must be packed");

                static_assert(
                    internal::can_cast<
                        v2::word<21>,
                        v2::word<21, v2::word_options::implicit>>::value,
                    "words should be castable even when one is implicit and one isn't"
                    );
            }
        }
        SECTION("units")
        {
            units::meters<v2::word<21, v2::word_options::packed>> v(5);

            v += 10;

            REQUIRE(v.count() == 15);
        }
    }
}
