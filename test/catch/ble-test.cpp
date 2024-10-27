#include <catch2/catch_all.hpp>

#include <embr/platform/ble/gatt/ets.h>

// NOTE: Limited to testing GATT/GAP BLE data types.

TEST_CASE("BLE")
{
    SECTION("Elapsed Time Service")
    {
        using et_type = embr::ble::gatt::ElapsedTime;
        embr::ble::gatt::ElapsedTime et;

        REQUIRE(sizeof(et) == 9);

        et.value = 123;
        et_type::rep_100ms v(et.value);
        constexpr estd::chrono::milliseconds compare_to(12300);
        // FIX: Can't convert since v won't present as an integer.  Also, doing so now
        // causes a ton of == overload issues
        estd::chrono::milliseconds v2(v);
        estd::chrono::milliseconds v3(et.as_100ms());

        REQUIRE(v2 == compare_to);
        REQUIRE(v == compare_to);
    }
}
