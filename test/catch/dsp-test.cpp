#include <catch2/catch_all.hpp>

#include <embr/dsp/precalc.h>
#include <embr/dsp/v1/drc.h>
#include <embr/dsp/v1/fp.h>
#include <embr/dsp/v1/phase.h>

using namespace embr;

template <dsp::v1::precalc_modes mode = dsp::v1::PRECALC_HALF, class T, std::size_t N>
void compare(const estd::span<T, N>& t, T v)
{
    T v1 = dsp::v1::sin_lookup<mode>(t, v);
    T v1_ = sin(v);

    CAPTURE(v1, v1_, v);
    REQUIRE(std::abs(std::abs(v1) - std::abs(v1_)) < 0.001);
}

constexpr const float float_buf1[] { 0, 1, 2, 3, 4, 5, 6 };

TEST_CASE("dsp")
{
    SECTION("precalc")
    {
        double* table = new double[8192];
        auto table2 = new float[4096];
        estd::span<double, 8192> t(table);
        estd::span<float, 4096> t2(table2);

        dsp::v1::init_sin_table(t);
        dsp::v1::init_sin_table(t2);

        compare(t, 0.2);
        compare(t, 1.0);
        compare(t, 3.0);
        compare(t, 9.0);
        compare(t, 32.0);

        compare(t2, 0.2f);

        dsp::v1::init_sin_table<dsp::PRECALC_QUART>(t2);

        compare<dsp::PRECALC_QUART>(t2, 0.2f);
        compare<dsp::PRECALC_QUART>(t2, 3.0f);
        compare<dsp::PRECALC_QUART>(t2, 6.0f);
        compare<dsp::PRECALC_QUART>(t2, 9.0f);
        compare<dsp::PRECALC_QUART>(t2, 32.0f);

        delete [] table;
        delete [] table2;
    }
    SECTION("phase")
    {
        dsp::v1::phase_generator<float> gen(0.1);
        dsp::v1::phase_generator<float> gen2 = dsp::v1::phase_generator<float>::from_freq(1146, 48000);

        REQUIRE(gen2.incr() >= 0.15);
        REQUIRE(gen2.incr() < 0.16);
        ++gen2;
        REQUIRE(gen2.incr() == gen2.phase());
    }
    SECTION("packed word")
    {
        using namespace dsp;

        detail::packed_word_base<PACKED_WORD_DEFAULT, estd::integer_sequence<unsigned, 4, 4>> pw1(1, 2);
        packed_word_default<5, 6, 5> pw2(1, 2, 3);
        detail::packed_word_base<PACKED_WORD_SWAP, estd::integer_sequence<unsigned, 5, 6, 5>> pw3(1, 2, 3);
        packed_word<PACKED_WORD_SWAP, 5, 6, 5> pw4(10, 15, 11);

        // TODO: Not ready yet
        //pw2 = pw3;
        //pw2 == pw3;

        REQUIRE(pw2.channel<0>() == 1);
        REQUIRE(pw2.channel<1>() == 2);
        REQUIRE(pw2.channel<2>() == 3);

        REQUIRE(pw3.channel<0>() == 1);
        REQUIRE(pw3.channel<1>() == 2);
        REQUIRE(pw3.channel<2>() == 3);

        REQUIRE(pw4.channel<0>() == 10);
        REQUIRE(pw4.channel<1>() == 15);
        REQUIRE(pw4.channel<2>() == 11);
    }
    SECTION("fixed point")
    {
        using fp8_24 = dsp::v1::fixed_point<8, 24>;
        using fp8 = dsp::v1::fixed_point<8, 8>;
        using fp16 = dsp::v1::fixed_point<16, 16>;

        static_assert(dsp::is_fixed_point<fp8>::value, "");
        static_assert(dsp::is_fixed_point<fp8_24>::value, "");
        static_assert(!dsp::is_fixed_point<int>::value, "");

        // Actual font support is in embr::gl - this is just fp verifications
        SECTION("lv font related")
        {
            using fp12_4 = dsp::v1::fixed_point<12, 4>;
            using fp12i_4 = dsp::v1::fixed_point<12, 4, dsp::v1::FP_SIGNED>;
            using fp4_4 = dsp::v1::fixed_point<4, 4, dsp::v1::FP_SIGNED>;

            static_assert(sizeof(fp4_4) == 1);
            static_assert(estd::is_same<fp12_4::value_type, uint16_t>::value, "");
            static_assert(estd::is_same<fp12_4::promoted_type, uint32_t>::value, "");

            static_assert(sizeof(fp12_4) == 2, "");
            static_assert(sizeof(fp12_4::value_type) == 2, "");
            static_assert(sizeof(fp12_4::promoted_type) == 4, "");

            fp12_4 v1{0x15};

            REQUIRE(v1.exp() == 1);
            REQUIRE(v1.man() == 5);
            REQUIRE(v1.as<float>() == 1.3125F);

            fp4_4 v2{char(0x94)};

            REQUIRE(v2.man() == 4);
            REQUIRE(int(v2.exp_u()) == 9);
            REQUIRE(v2.as<float>() == -6.75);
            // DEBT: Presumes 2's complement
            REQUIRE(int(v2.exp()) == -7);

            // TODO: Not ready
            //lv::fp12_4 v3 = v1 * v2;

            fp12_4 v3(v1);

            v3 *= v1;

            // Close as resolution will allow us to 1.3125F^2
            REQUIRE(v3.as<float>() == 1.6875f);

            v1 -= v2;

            REQUIRE(v1.as<float>() == 8.0625f);

            //REQUIRE(v3.num_s() == -7);

            v3 /= v1;

            REQUIRE(v3.as<float>() == 0.1875f);
        }
        SECTION("general")
        {
            static_assert(std::is_same<fp8_24::value_type, uint32_t>::value, "");
            static_assert(std::is_same<fp8_24::promoted_type, uint64_t>::value, "");

            fp8_24 v1{0x1800000};
            fp8 v2{0x0180};

            REQUIRE(v1.as<float>() == 1.5f);
            REQUIRE(v2.as<float>() == 1.5f);

            // Mixed precision not quite ready yet
            /*
            v1 *= v2;

            REQUIRE(v1.as<float>() == 2.25f);

            v2 *= v1;

            REQUIRE(v2.as<float>() == 3.375f);  */

            auto v3 = fp16::from(1.5F);
            static constexpr auto v4 = fp16::from(1.5);

            REQUIRE(v3.exp() == 1);
            REQUIRE(v3.man() == 0x8000);
        }
        SECTION("common_type")
        {
            using ct1 = estd::common_type_t<fp8, fp16>;

            static_assert(ct1::exponent == 16, "");
            static_assert(ct1::mantissa == 16, "");
        }
        SECTION("operator /")
        {
            using fp4_12 = dsp::v1::fixed_point<4, 12, dsp::v1::FP_SIGNED>;

            auto v = fp4_12::from(0.1);

            v = v / 100;

            REQUIRE_THAT(v.as<float>(), Catch::Matchers::WithinAbs(0.0009765, .000001));
        }
        SECTION("operator *")
        {
            using fp8_8 = dsp::v1::fixed_point<8, 8, dsp::v1::FP_SIGNED>;

            auto v = fp8_8::from(0.1);

            v = v * 100;

            REQUIRE_THAT(v.as<float>(), Catch::Matchers::WithinAbs(9.765, .001));
        }
    }
    SECTION("drc")
    {
        constexpr float _thresh = 0.7;
        constexpr float _attack = 0.8;
        constexpr float _release = 0.01;

        SECTION("float")
        {
            float out[16] {};
            dsp::drc<float> drc1;
            using params = dsp::drc<float>::params;
            float v{1};

            constexpr params p1{ _thresh, _attack, _release };

            v = drc1.process(v, p1);

            REQUIRE_THAT(v, Catch::Matchers::WithinAbs(0.875, .001));

            v = drc1.process(v, p1);

            REQUIRE_THAT(v, Catch::Matchers::WithinAbs(0.712, .001));

            drc1.reset();

            process(
                drc1, { 1.0f, 0.2f, 0.5f },
                float_buf1, std::end(float_buf1), out);

            REQUIRE(out[0] == 0);
            REQUIRE(out[1] == 1);
            REQUIRE(out[2] == 2);
            REQUIRE_THAT(out[3], Catch::Matchers::WithinAbs(2.862, .001));
            REQUIRE_THAT(out[4], Catch::Matchers::WithinAbs(2.441, .001));
        }
        SECTION("fp")
        {
            using fp4_12 = dsp::v1::fixed_point<4, 12>;

            fp4_12 v;

            constexpr auto thresh = fp4_12::from(_thresh);
            constexpr auto attack = fp4_12::from(_attack);
            constexpr auto release = fp4_12::from(_release);

            dsp::drc<fp4_12> drc1;

            // No std::abs compatibility for fp yet
            //v = drc1.process(v, thresh, attack, release);
        }
    }
}
