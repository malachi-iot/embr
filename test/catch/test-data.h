/**
 *
 * References:
 *
 * 1. https://embeddeduse.com/2020/01/17/introduction-to-the-sae-j1939-standard/
 * 2. https://chortle.ccsu.edu/assemblytutorial/Chapter-15/ass15_3.html
 * 3. bits/README.md
 * 4. J1939-81 (MAY2003)
 */
#pragma once

#include <estd/cstdint.h>

// eec1 examples are from [1]
constexpr static const uint8_t eec1_id_example[] = { 0xc, 0xf0, 0x04, 0x00 };
constexpr static const uint8_t eec1_frame_example1[] =
    {0x62, 0xc5, 0x49, 0x28, 0x42, 0x13, 0x07, 0xd3 };

// [2]
constexpr static const uint8_t be_example1[] = { 0x12, 0x34, 0x56, 0x78 };
constexpr static const uint8_t le_example1[] = { 0x78, 0x56, 0x34, 0x12 };
constexpr static const uint32_t endian_example1 = 0x12345678;

// [3] 2.1.1
constexpr static const uint8_t be_example2_1_1[] = { 0b1010011, 0b11000101 };
constexpr static const uint16_t endian_example2_1_1 = 0b01010110;

// [3] 2.1.3.1
constexpr static const uint8_t le_example2_1_3_1[] = { 0b10100000, 0b00000101 };
constexpr static const uint8_t le_example2_1_3_1_32bit[] = { 0b10100000, 0b00000101, 0, 0 };
constexpr static const uint16_t endian_example2_1_3_1 = 0b1011010;
constexpr static const uint8_t le_example2_1_3_1_aug[] = { 0b10100000, 0b00110011, 0b00000101 };
constexpr static const uint16_t endian_example2_1_3_1_aug = 0b101001100111010;


namespace test_data {

// 0xBF 81 - arbitrary test bytes
static uint8_t buf1[] = { 0b10111111, 0b10000001 };

}

namespace test {

inline void compare(const uint8_t lhs[], const uint8_t rhs[], size_t count)
{
    for(int i = 0; i < count; i++)
        REQUIRE(lhs[i] == rhs[i]);
}

}