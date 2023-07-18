/**
 * References:
 *
 * 1. Reserved
 * 2. bits/README.md v0.3
 */
#define FEATURE_EMBR_WORD_STRICTNESS 0

#include <estd/algorithm.h>

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

static void test_word_3()
{
    internal::word<29> can_id(0x98fdcc77);

    constexpr bits::descriptor d1(8, 18);
    unsigned v;

    // j1939 extended pdu range
    v = can_id.get(d1);

    TEST_ASSERT_EQUAL(0xFDCC, v);
}

#if __cplusplus >= 201103L
void test_getter_1()
{
    byte bytes[] { 0x12, 0x34 };

    auto v = getter<little_endian, lsb_to_msb>::get<0, 16, unsigned>(bytes);

    TEST_ASSERT_EQUAL(0x3412, v);
}

void test_setter_1a()
{
    byte bytes[8] = {};

    // Mates to [2] 2.1.3.1.
    constexpr uint16_t v = 0x5A;        // 1011010

    setter<little_endian, lsb_to_msb>::set<4, 7>(bytes, v);

    TEST_ASSERT_EQUAL(0xA0, bytes[0]);  // 1010xxxx
    TEST_ASSERT_EQUAL(0x05, bytes[1]);  // .....101

    estd::fill_n(bytes, sizeof(bytes), 0xFF);

    setter<little_endian, lsb_to_msb>::set<4, 7>(bytes, v);

    TEST_ASSERT_EQUAL(0xAF, bytes[0]);  // 10101111
    TEST_ASSERT_EQUAL(0xFD, bytes[1]);  // 11111101
}

void test_setter_1b()
{
    // Mates to [2] 2.1.3.1.
    //constexpr uint16_t v = 0x5A;        // 1011010
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
    RUN_TEST(bits::test_word_3);

#if __cplusplus >= 201103L
    RUN_TEST(bits::test_getter_1);
    RUN_TEST(bits::test_setter_1a);
    RUN_TEST(bits::test_setter_1b);
#endif
}
