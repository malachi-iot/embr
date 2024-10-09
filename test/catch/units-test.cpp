#include <catch2/catch_all.hpp>

#include <estd/sstream.h>
#include <estd/internal/units/ostream.h>
#include <estd/internal/units/bytes.h>  // Just for sanity check make sure si specializations don't collide
#include <estd/internal/units/operators.hpp>

#include <embr/units/celsius.h>
#include <embr/units/decibels.h>
#include <embr/units/feet.h>
#include <embr/units/frequency.h>
#include <embr/units/meters.h>
#include <embr/units/pascals.h>
#include <embr/units/rpm.h>
#include <embr/units/volts.h>
#include <embr/units/watts.h>
#include <embr/units/weight.h>
#include <embr/units/volume.h>


using namespace embr;
using namespace embr::units;

TEST_CASE("units")
{
    estd::layer1::ostringstream<256> out;
    const auto& s = out.rdbuf()->str();

    SECTION("amps")
    {
        SECTION("double")
        {
            // DEBT: Due to nasty to-double-string kludge, we have
            // to account for rounding error here
            units::amps<double> v(5.100000001);

            out << estd::put_unit(v, false);

            REQUIRE(s == "5.10 amps");
        }
        SECTION("int")
        {
            milliamps<uint16_t> ma{3600};
            // DEBT: Can't do an = here, but would like to.  This requires a default ctor
            amps<uint8_t> a(ma);

            REQUIRE(a.count() == 3);

            write(out, ma);

            REQUIRE(s == "3600 milliamps");
        }
    }
    SECTION("celsius")
    {
        // lifted right from SPN 4420
        celsius<uint8_t, estd::ratio<1>, estd::internal::units::adder<int16_t, -40> > c1{0};
        celsius<int> c2{c1};

        REQUIRE(c1.count() == -40);
        REQUIRE(c2.count() == -40);

        write_abbrev(out, c1);

        REQUIRE(out.rdbuf()->str() == "-40 deg C");
    }
    SECTION("decibels")
    {
        decibels<int> d1{5};

        write_abbrev(out, d1);

        REQUIRE(s == "5dB");
    }
    SECTION("feet")
    {
        yards<int> y{10};
        feet<int> f{y};

        REQUIRE(f.count() == 30);
    }
    SECTION("meters")
    {
        SECTION("km")
        {
            kilometers<int> km{32};
            meters<int> m{km};

            REQUIRE(m.count() == 32000);

            write_abbrev(out, km);

            REQUIRE(s == "32km");
        }
    }
    SECTION("rpm")
    {
        rpm<uint16_t> rpm1{5000};
        rpm<uint16_t, estd::ratio<1, 2> > rpm2{rpm1};

        REQUIRE(rpm2.count() == 10000);

        write_abbrev(out, rpm1);

        REQUIRE(out.rdbuf()->str() == "5000rpm");
    }
    SECTION("volts")
    {
        SECTION("double")
        {
            units::volts<double> v(5);

            out << estd::put_unit(v);

            REQUIRE(s == "5.00V");

            v += 0.5;

            REQUIRE(v > 5.049);
        }
        SECTION("int")
        {
            millivolts<uint16_t> mv{3600};
            decivolts<uint8_t> dv{mv};
            volts<uint8_t> v{mv};

            REQUIRE(dv.count() == 36);
            REQUIRE(v.count() == 3);

            write(out, dv);

            REQUIRE(out.rdbuf()->str() == "36 decivolts");

            microvolts<uint32_t> uv = mv + dv;

            out.rdbuf()->clear();

            write(out, uv);

            REQUIRE(out.rdbuf()->str() == "7200000 microvolts");
        }
    }
    SECTION("watts")
    {
        watts<uint16_t> w{1000};

        write(out, w);

        REQUIRE(s == "1000 watts");
    }
    SECTION("conversions")
    {
        volts<uint8_t> v{120};
        watts<uint16_t> w{1000};

        // EXPERIMENTAL, not ready
        //milliamps<uint16_t> ma = w / v;

        // FIX: We want to prohibit this
        //v = a;
    }
    SECTION("rates/speed")
    {
        // NOTE: Compound units like this aren't quite fleshed out yet

        SECTION("meters per second")
        {
            constexpr kilometers_per_hour<uint8_t> kph(60);
            meters_per_second<uint16_t> m_s{kph};

            REQUIRE(m_s.count() == 60000 / 3600);
        }
    }
    SECTION("misc")
    {
        // DEBT: outlier tests that belong in estd
        typedef estd::chrono::duration<uint8_t, estd::ratio<6, 10000>> type1;

        type1 d1(0), d2(255);

        REQUIRE(d1 == estd::chrono::milliseconds(0));
        REQUIRE(d2 == estd::chrono::milliseconds(153));
    }
}
