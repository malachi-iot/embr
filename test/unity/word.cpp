#define FEATURE_EMBR_WORD_STRICTNESS 0

#include <embr/word.h>

#include "unit-test.h"

static void test_word_16bit()
{
    {
        embr::word<16> w(5);

        TEST_ASSERT_EQUAL(5, w.cvalue());
    }
}

static void test_word_32bit()
{
    {
        embr::word<32> w(5);

        TEST_ASSERT_EQUAL(5, w.cvalue());
    }

    {
        embr::word<24> w(5);

        TEST_ASSERT_EQUAL(5, w.cvalue());
    }
}

#ifdef ESP_IDF_TESTING
TEST_CASE("bit manipulator tests", "[bits]")
#else
void test_word()
#endif
{
    RUN_TEST(test_word_16bit);
    RUN_TEST(test_word_32bit);
}
