#define FEATURE_EMBR_WORD_STRICTNESS 0

#include <embr/bits/word.hpp>
#if __cplusplus >= 201103L
#include <embr/bits/bits.hpp>
#endif

#include "unit-test.h"

namespace bits {

using namespace embr::bits;

struct test_word : internal::word<10>
{
    typedef internal::word<10> base_type;

    EMBR_BITS_WORD_PROPERTY(val1, 0, 4);

    EMBR_BITS_WORD_GETTER(val2, 4, 6);
    EMBR_BITS_WORD_SETTER(val2, 4, 6);

    test_word() : base_type(0) {}
};

static void test_word_1()
{
    internal::word<10> w(1);

    embr::word<1> val = w.get<0, 1>();

    TEST_ASSERT_EQUAL(1, val.value());

    w.set<4, 6>(3);

    TEST_ASSERT_EQUAL(0x31, w.value());
}

static void test_word_2()
{
    test_word w;

    TEST_ASSERT_EQUAL(0, w.val2().value());

    w.val1(0x1);
    w.val2(0x3);

    TEST_ASSERT_EQUAL(0x31, w.value());
    TEST_ASSERT_EQUAL(0x1, w.val1().value());
    TEST_ASSERT_EQUAL(0x3, w.val2().value());
}

#if __cplusplus >= 201103L
void test_getter_1()
{
    byte bytes[] { 0x12, 0x34 };

    auto v = getter<little_endian, lsb_to_msb>::get<0, 16, unsigned>(bytes);

    TEST_ASSERT_EQUAL(0x3412, v);
}
#endif

}

#ifdef ESP_IDF_TESTING
TEST_CASE("bit manipulator tests", "[bits]")
#else
void test_bits()
#endif
{
    RUN_TEST(bits::test_word_1);
    RUN_TEST(bits::test_word_2);
#if __cplusplus >= 201103L
    RUN_TEST(bits::test_getter_1);
#endif
}
