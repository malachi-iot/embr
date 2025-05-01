#include <embr/platform/ble/gatt/ots.h>

#include "unit-test.h"

using namespace embr::ble;

// No physical/transport things here, just data structures

using oacp_type = gatt::v1::ObjectActionControlPoint;


static void test_ots_oacp_checksum()
{
    oacp_type oacp;

    oacp.opcode = oacp_type::CHECKSUM;
    oacp.checksum.length = 10;
    oacp.checksum.offset = 10;
}

static void test_ots_oacp_response()
{
    oacp_type oacp
    {
        .opcode = oacp_type::RESPONSE_CODE,
        .response
        {
            .opcode = oacp_type::CREATE,
            .result_code = oacp_type::SUCCESS,
            // DEBT: Find a way to squish the need for parameter{}
            .parameter {}
        }
    };
}

#ifdef ESP_IDF_TESTING
TEST_CASE("ble tests", "[ble]")
#else
void test_ble()
#endif
{
    RUN_TEST(test_ots_oacp_checksum);
    RUN_TEST(test_ots_oacp_response);
}