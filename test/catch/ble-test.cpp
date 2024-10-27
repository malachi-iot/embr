#include <catch2/catch_all.hpp>

#include <embr/platform/ble/gatt/ets.h>

// NOTE: Limited to testing GATT/GAP BLE data types.

TEST_CASE("BLE")
{
    SECTION("Elapsed Time Service")
    {
        embr::ble::gatt::ElapsedTime et;

        REQUIRE(sizeof(et) == 9);
    }
}
