#define FEATURE_EMBR_WORD_STRICTNESS 0

#include <embr/bits/word.hpp>

#include "unit-test.h"

namespace bits {

using namespace embr::bits;

static void test_word_1()
{
    internal::word<10> w(0);

    //int val = w.get<0, 0>();
}

}

#ifdef ESP_IDF_TESTING
TEST_CASE("bit manipulator tests", "[bits]")
#else
void test_bits()
#endif
{
    RUN_TEST(bits::test_word_1);
}
