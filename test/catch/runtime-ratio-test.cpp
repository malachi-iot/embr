#include <catch.hpp>

#include <embr/internal/runtime-ratio.h>

TEST_CASE("Runtime ratio", "[ratio]")
{
    using namespace embr::internal;

    SECTION("converter (runtime default 80)")
    {
        DurationConverter<int, -1> converter;

        uint64_t v = converter.convert(estd::chrono::milliseconds(10));

        REQUIRE(v == 10000);

        v = converter.convert(estd::chrono::duration<uint16_t, estd::ratio<1, 3> >(1));
        REQUIRE(v == 333333);

        // 5/33 of a second = 0.151515151_ forever
        v = converter.convert(estd::chrono::duration<uint16_t, estd::ratio<5, 33> >(1));
        REQUIRE(v == 151515);

        SECTION("low precision")
        {
            typedef estd::chrono::duration<uint8_t, estd::milli> duration;
            v = converter.convert(duration(50));

            REQUIRE(v == 50000);

            v = converter.convert(duration::max());

            REQUIRE(v == 255000);
        }
    }
    SECTION("converter (runtime 400)")
    {
        DurationConverter<int, -1> converter;

        converter.numerator(400);

        uint64_t v = converter.convert(estd::chrono::milliseconds(10));

        REQUIRE(v == 2000);
    }
    SECTION("converter (runtime 8000)")
    {
        DurationConverter<int, -1> converter;

        converter.numerator(8000);

        uint64_t v = converter.convert(estd::chrono::milliseconds(100));

        REQUIRE(v == 1000);

        estd::chrono::duration<int, std::ratio<3, 1000> > v2;

        converter.convert(v, &v2);

        REQUIRE(v2.count() == 33);
    }
    SECTION("constexpr converter (80)")
    {
        DurationConverter<int, 80> converter;
        uint64_t v;

        SECTION("basic")
        {
            v = converter.convert(estd::chrono::milliseconds(10));

            REQUIRE(v == 10000);
        }
        SECTION("edge case")
        {
            v = converter.convert(estd::chrono::duration<uint16_t, estd::ratio<1, 3> >(1));
            REQUIRE(v == 333333);

            // 5/33 of a second = 0.151515151_ forever
            v = converter.convert(estd::chrono::duration<uint16_t, estd::ratio<5, 33> >(1));
            REQUIRE(v == 151515);
        }
        SECTION("low precision")
        {
            typedef estd::chrono::duration<uint8_t, estd::milli> duration;
            v = converter.convert(duration(50));

            REQUIRE(v == 50000);

            v = converter.convert(duration::max());

            REQUIRE(v == 255000);
        }
    }
    SECTION("constexpr converter (400)")
    {
        DurationConverter<int, 400> converter;

        uint64_t v = converter.convert(estd::chrono::milliseconds(10));

        REQUIRE(v == 2000);
    }
    SECTION("multiply")
    {
        SECTION("2/8 x 3/2")
        {
            // 2/8 * 3/2 = 1/4 * 3/2 = 3/8
            runtime_ratio<uint8_t, 2, runtime_ratio_den> lhs{8};

            auto v = runtime_multiply(lhs, estd::ratio<3, 2>());

            REQUIRE(v.num == 3);
            REQUIRE(v.den == 8);
        }
        SECTION("80,000,000/80 x 1/1000")
        {
            // 80,000,000/80 x 1/1000 = 1,000,000 x 1/1000 = 1000
            // although currently, algorithm does:
            // 80,000,000/80 x 1/1000 = 80,000/80 x 1/1
            runtime_ratio<uint32_t, 80000000, runtime_ratio_den> lhs{80};

            auto v = runtime_multiply(lhs, estd::milli());

            // 'v' is not auto reduced, so it's coming back as 80,000:80
            //REQUIRE(v.num == 1000);
            //REQUIRE(v.den == 1);

            runtime_ratio<uint32_t, 80000000, runtime_ratio_den, 1> lhs2{80};
            auto v2 = lhs.multiply(estd::milli());
            // Catch doesn't like referencing constexpr static member variables directly
            constexpr int num = v2.num;

            REQUIRE(num == 80000);
            REQUIRE(v2.den == 80);

            // Downside to doing this is a runtime gcd and a full num+den runtime ratio
            auto reduced = reduce(v2);

            REQUIRE(reduced.num == 1000);
            REQUIRE(reduced.den == 1);
        }
        SECTION("runtime numerator")
        {
            // 10/1000
            runtime_ratio<uint32_t, 1000, runtime_ratio_num> r{10};
            typedef decltype(r) r_type;

            int num, den;;

            // lhs den (1000) and rhs num (2) reduce against one another
            typedef r_type::mult_reducer<2> reduced2;

            num = reduced2::num;
            den = reduced2::den;

            REQUIRE(num == 1);
            REQUIRE(den == 500);

            // lhs den (1000) and rhs num (10) reduce against one another
            typedef r_type::mult_reducer<10> reduced;

            num = reduced::num;
            den = reduced::den;

            REQUIRE(num == 1);
            REQUIRE(den == 100);

            // 10/1000 x 10/2 = 100/2000 = 1/20

            // 10/2
            // lhs den (reduced) * rhs den ->
            // 100 * 2
            den = r_type::mult_helper<10, 2>();

            REQUIRE(den == 200);

            auto v = r.multiply(estd::ratio<10, 2>{});
            den = v.den;

            REQUIRE(v.num == 10);
            REQUIRE(den == 200);

            auto _reduced = reduce(v);

            REQUIRE(_reduced.num == 1);
            REQUIRE(_reduced.den == 20);
        }
    }
    SECTION("convert durations with runtime ratio")
    {
        unsigned val = 100;  // milliseconds

        // aka microseconds
        runtime_ratio<uint32_t, 80000000, runtime_ratio_den> lhs{80};
        runtime_ratio<uint32_t, estd::milli::den, runtime_ratio_num> rhs{(uint32_t)estd::milli::num * val};

        REQUIRE(rhs.num == val);

        // converter is the ratio needed to multiply against to convert to esp32 us counter from
        // milliseconds
        auto converter = lhs.multiply(estd::milli{});

        auto val2 = converter.num * val / converter.den;

        REQUIRE(val2 == 100000);

        auto v = lhs.multiply(rhs);

        val2 = v.num / v.den;

        REQUIRE(val2 == 100000);

        // EXPERIMENTAL int-only flavor
        val2 = converter.multiply(val);

        REQUIRE(val2 == 100000);
    }
}
