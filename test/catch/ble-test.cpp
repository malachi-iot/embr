#include <catch2/catch_all.hpp>

#include <embr/platform/ble/gatt/ets.h>
#include <embr/platform/ble/gatt/char/date-time.h>
#include <embr/platform/ble/gatt/ots.h>

// NOTE: Limited to testing GATT/GAP BLE data types.

TEST_CASE("BLE")
{
    using namespace embr::ble;

    SECTION("Elapsed Time Service")
    {
        using et_type = embr::ble::gatt::ElapsedTime;
        embr::ble::gatt::ElapsedTime et;

        REQUIRE(sizeof(et) == 9);

        et.value = 123;
        et_type::rep_100ms v(et.value);
        constexpr estd::chrono::milliseconds compare_to(12300);
        estd::chrono::milliseconds v2(v);
        estd::chrono::milliseconds v3(et.as_100ms());

        REQUIRE(v2 == compare_to);
        REQUIRE(v == compare_to);
        REQUIRE(v3 == compare_to);
    }
    SECTION("gatt characteristics")
    {
        SECTION("date time")
        {
            gatt::DateTime dt;
        }
        SECTION("current elapsed time")
        {
            REQUIRE(sizeof(gatt::CurrentElapsedTime) == 11);
        }
        SECTION("object access control point")
        {
            using type = gatt::ObjectActionControlPoint;
            type oacp;
            oacp.opcode = type::CHECKSUM;
            oacp.checksum.length = 10;
            oacp.checksum.offset = 10;
            auto raw = (const uint8_t*) &oacp;

            REQUIRE(raw[0] == 3);
            REQUIRE(raw[1] == 10);
            REQUIRE(raw[2] == 0);
            REQUIRE(raw[5] == 10);
            REQUIRE(raw[6] == 0);
        }
    }
}
