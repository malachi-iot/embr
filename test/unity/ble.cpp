#include <embr/platform/ble/gatt/ots.h>

#include "unit-test.h"

using namespace embr::ble;

// No physical/transport things here, just data structures

static void test_ots()
{
    /* FIX: Doesn't work due (probably) to non-trivial uint32 
    union
    {
        //gatt::v1::ObjectActionControlPoint::Create oacp_create;

        gatt::uint32 v;
    };
    */
}

#ifdef ESP_IDF_TESTING
TEST_CASE("ble tests", "[ble]")
#else
void test_ble()
#endif
{
    RUN_TEST(test_ots);
}