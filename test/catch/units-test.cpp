#include <catch2/catch.hpp>

#include <estd/sstream.h>
#include <estd/internal/units/ostream.h>

#include <embr/units/volts.h>
#include <embr/units/watts.h>

using namespace embr;

TEST_CASE("units")
{
    estd::layer1::ostringstream<256> out;
    const auto& s = out.rdbuf()->str();

    SECTION("amps")
    {
        // DEBT: Due to nasty to-double-string kludge, we have
        // to account for rounding error here
        units::amps<double> v(5.100000001);

        out << estd::put_unit(v, false);

        REQUIRE(s == "5.10 amps");
    }
    SECTION("volts")
    {
        units::volts<double> v(5);

        out << estd::put_unit(v);

        REQUIRE(s == "5.00V");
    }
    SECTION("watts")
    {

    }
}