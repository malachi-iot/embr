#define FEATURE_EMBR_WORD_STRICTNESS 0

#include <embr/word.h>

#include "unit-test.h"

template <unsigned N, typename TInt>
static void test_word_basics()
{
    typedef embr::word<N> word_type;

    TEST_ASSERT_EQUAL(
        estd::numeric_limits<TInt>::digits,
        estd::numeric_limits<typename word_type::type>::digits);

    word_type w(5);

    TEST_ASSERT_EQUAL(5, w.value());

    w <<= 1;

    TEST_ASSERT_EQUAL(10, w.value());

    w |= embr::word<1>(1);

    TEST_ASSERT_EQUAL(11, w.value());
}

static void test_word_16bit()
{
    test_word_basics<16, uint16_t>();
    test_word_basics<11, uint16_t>();
}

static void test_word_32bit()
{
    {
        test_word_basics<32, uint32_t>();
    }

    {
        test_word_basics<24, uint32_t>();
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
