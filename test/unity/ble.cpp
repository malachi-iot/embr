#include <embr/platform/ble/gatt/ots.h>

#include "unit-test.h"

using namespace embr::ble;

// No physical/transport things here, just data structures

static void test_ots()
{
    using oacp_type = gatt::v1::ObjectActionControlPoint;

    oacp_type oacp;

    oacp.opcode = oacp_type::CHECKSUM;
    oacp.checksum.length = 10;
    oacp.checksum.offset = 10;
}

#ifdef ESP_IDF_TESTING
TEST_CASE("ble tests", "[ble]")
#else
void test_ble()
#endif
{
    RUN_TEST(test_ots);
}