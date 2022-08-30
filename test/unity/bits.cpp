#define FEATURE_EMBR_WORD_STRICTNESS 0

#include <embr/bits/word.hpp>

#include "unit-test.h"

static void test_bits_1()
{

}

#ifdef ESP_IDF_TESTING
TEST_CASE("bit manipulator tests", "[bits]")
#else
void test_bits()
#endif
{
    RUN_TEST(test_bits_1);
}
