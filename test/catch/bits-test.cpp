/**
 * References:
 *
 * 1. https://github.com/malachib/chariot/tree/main/lib/j1939/j1939 v0.1
 * 2. README.md
 */
#include <catch.hpp>

#include <embr/bits/bits.hpp>

#include "test-data.h"

using namespace embr;

void compare_le_example1(const uint8_t raw[])
{
    test::compare(raw, le_example1, 4);
}

TEST_CASE("bits2")
{
    uint8_t raw[64];
    std::fill_n(raw, sizeof(raw), 0);

    SECTION("byte operations")
    {
        SECTION("lsb_to_msb")
        {
            SECTION("get")
            {
                uint8_t v =
                    bits::get<bits::no_endian, uint8_t, bits::lsb_to_msb>(bits::descriptor{ 0, 4}, be_example1);
                REQUIRE(v == 2);
            }
            SECTION("set")
            {
                constexpr uint8_t v = 2;
                bits::set<bits::no_endian>(bits::descriptor{ 4, 4}, raw, v);
                REQUIRE(raw[0] == 0x20);
            }

            // DEBT: Need deeper setter tests to more deeply test masking
        }
        SECTION("msb_to_lsb")
        {
            SECTION("get")
            {
                int v =
                    bits::get<bits::no_endian, uint8_t, bits::msb_to_lsb>(bits::descriptor{ 3, 4}, be_example1);
                REQUIRE(v == 2);
            }
            SECTION("set")
            {
                constexpr uint8_t v = 2;
                bits::set<bits::no_endian, uint8_t, bits::msb_to_lsb>(bits::descriptor{ 7, 4}, raw, v);
                REQUIRE(raw[0] == 0x20);
            }
        }
        SECTION("encoder")
        {
            // just for full test coverage.  We are confident these operate by virtue of the bits::set
            // call and the below word-based tests

            // Endian doesn't matter for byte-only tests
            bits::encoder<bits::big_endian, bits::lsb_to_msb, bits::msb_to_lsb> e{raw};

            e.set<uint8_t>(0, bits::descriptor{4, 3}, 3);

            REQUIRE(raw[0] == 0x30);
        }
    }
    SECTION("word operations")
    {
        SECTION("big endian")
        {
            SECTION("encoder, lsb_to_msb")
            {
                bits::encoder<bits::big_endian> e(&raw[0]);

                SECTION("32-bit")
                {
                    // TBD
                    //e.set(0, bits::descriptor{4, 8}, 0x13);
                }
            }
            SECTION("decoder, lsb_to_msb")
            {
                bits::decoder<bits::big_endian> d{be_example1};

                SECTION("16-bit")
                {
                    //d.get<uint8_t>(1, bits::descriptor{1, 4});
                    auto v = d.get<uint16_t>(0, bits::descriptor{4, 8});

                    REQUIRE(v == 0x13);
                }
                SECTION("32-bit")
                {
                    auto v = d.get<uint32_t>(0, bits::descriptor{4, 8});

                    REQUIRE(v == 0x13);
                }
            }
            SECTION("decoder, msb_to_lsb")
            {
                bits::decoder<bits::big_endian, bits::msb_to_lsb> d{be_example1};

                SECTION("16-bit")
                {
                    auto v = d.get<uint16_t>(0, bits::descriptor{3, 8});

                    REQUIRE(v == 0x23);
                }
                SECTION("32-bit, length=8")
                {
                    auto v = d.get<uint32_t>(0, bits::descriptor{3, 8});

                    REQUIRE(v == 0x23);
                }
                SECTION("32-bit, length=20")
                {
                    auto v = d.get<uint32_t>(0, bits::descriptor{3, 20});

                    REQUIRE(v == 0x23456);
                }
            }
        }
        SECTION("little endian")
        {
            // [1] 2.1.3
            SECTION("decoder, lsb_to_msb")
            {
                bits::decoder<bits::little_endian, bits::lsb_to_msb, bits::msb_to_lsb> d{le_example1};

                SECTION("16-bit")
                {
                    auto v = d.get<uint16_t>(2, bits::descriptor{0, 16});

                    REQUIRE(v == 0x1234);
                }
                SECTION("16-bit, bitpos=4, length=8")
                {
                    auto v = d.get<uint16_t>(2, bits::descriptor{4, 8});

                    REQUIRE(v == 0x13);
                }
                SECTION("32-bit, length=20")
                {
                    auto v = d.get<uint32_t>(0, bits::descriptor{4, 20});

                    REQUIRE(v == 0x34567);
                }
                SECTION("32-bit, length=28")
                {
                    auto v = d.get<uint32_t>(0, bits::descriptor{4, 28});

                    REQUIRE(v == 0x1234567);
                }
                SECTION("32-bit")
                {
                    auto v = d.get<uint32_t>(0);

                    REQUIRE(v == 0x12345678);
                }
            }
            // [1] 2.1.3
            SECTION("encoder, lsb_to_msb")
            {
                bits::encoder<bits::little_endian, bits::lsb_to_msb, bits::msb_to_lsb> e{raw};

                SECTION("16-bit, bitpos=0, length=12")
                {
                    constexpr uint16_t v = 0x123;

                    e.set(0, bits::descriptor{0, 12}, v);

                    REQUIRE(raw[0] == 0x23);
                    REQUIRE(raw[1] == 0x10);
                }
                SECTION("16-bit, bitpos=4, length=12")
                {
                    constexpr uint16_t v = 0x123;

                    e.set(0, bits::descriptor{4, 12}, v);

                    REQUIRE(raw[0] == 0x30);
                    REQUIRE(raw[1] == 0x12);
                }
                SECTION("32-bit, bitpos=0, length=12")
                {
                    constexpr uint32_t v = 0x123;

                    e.set(0, bits::descriptor{0, 12}, v);

                    REQUIRE(raw[0] == 0x23);
                    REQUIRE(raw[1] == 0x10);
                }
                SECTION("32-bit, bitpos=4, length=12")
                {
                    constexpr uint32_t v = 0x123;

                    e.set(0, bits::descriptor{4, 12}, v);

                    REQUIRE(raw[0] == 0x30);
                    REQUIRE(raw[1] == 0x12);
                }
                SECTION("32-bit")
                {
                    e.set(0, endian_example1);

                    compare_le_example1(raw);
                }
            }
        }
    }
    SECTION("material")
    {
        SECTION("big endian")
        {
            std::copy_n(be_example1, sizeof(be_example1), raw);

            bits::material<bits::big_endian> item{raw};

            REQUIRE(!(item < item));
            REQUIRE(!(item > item));
            REQUIRE((item == item));
        }
        SECTION("little endian")
        {
            std::copy_n(le_example1, sizeof(le_example1), raw);

            bits::material<bits::little_endian> item{raw};

            REQUIRE(!(item < item));
            REQUIRE(!(item > item));
            REQUIRE((item == item));
        }
    }
    SECTION("layer1 operations")
    {
        bits::layer1::encoder<bits::big_endian, 4, bits::lsb_to_msb, bits::msb_to_lsb> e;

        e.set<uint8_t>(0, bits::descriptor{4, 3}, 3);

        REQUIRE(e.data()[0] == 0x30);
    }
    SECTION("word operations")
    {
        constexpr int v1 = 5;
        constexpr int v2 = 10;
        constexpr bool v3 = true;

        SECTION("8-bit")
        {
            bits::internal::word<8> w{0};

            constexpr bits::internal::word<8>::descriptor_type d1{1, 3};
            constexpr bits::descriptor d2{4, 4};

            SECTION("getter/setter")
            {
                w.set(d1, v1);

                REQUIRE(w.get(d1) == v1);
                REQUIRE(w.get(d2) == 0);

                w.set(d2, v2);

                REQUIRE(w.get(d1) == v1);
                REQUIRE(w.get(d2) == v2);

                REQUIRE(w.get<1, 3>() == v1);
            }
            SECTION("indexer")
            {
                auto i = w[d1];

                i = v1;
                REQUIRE(i.value() == v1);
            }
        }
        SECTION("16-bit")
        {
            bits::internal::word<16> w{0};
            constexpr bits::descriptor d1{3, 4};
            constexpr bits::descriptor d2{8, 4};
            constexpr bits::descriptor d3{2, 1};

            SECTION("getter/setter")
            {
                w.set(d1, v1);

                REQUIRE(w.get(d1) == v1);
                REQUIRE(w.get(d2) == 0);
                bool v = estd::to_integer<bool>(w.get<d3.bitpos>());
                REQUIRE(!v);

                //w.set<d3.bitpos>(bits::experimental::word<8>((uint8_t)1));
                w.set<d3.bitpos>(1);
                //w.set(d3, true);

                w.set(d2, v2);

                REQUIRE(w.get(d1) == v1);
                REQUIRE(w.get(d2) == v2);
                REQUIRE(w.get<d3.bitpos>() == true);
            }
            SECTION("indexer")
            {
                auto i = w[d1];

                i = v1;
                REQUIRE(i.value() == v1);
            }
        }
    }
    SECTION("experimental")
    {
        SECTION("bit_traits")
        {
            bits::internal::word<16> v{0};
            typedef bits::experimental::bit_traits<3, 10> traits_type;
            bits::descriptor d{3, 10};
            constexpr unsigned v1 = 77;

            v.set(d, v1);

            REQUIRE(v.get<traits_type>() == v1);

            REQUIRE(d == traits_type::get_descriptor());
        }
    }
    SECTION("comparison")
    {
        uint8_t compare_to[32];

        SECTION("little endian")
        {
            raw[1] = 0x12;
            raw[0] = 0x34;

            compare_to[1] = 0x13;
            compare_to[0] = 0x34;

            SECTION("gt1")
            {
                bool result = bits::internal::greater_than_le(&compare_to[0], &compare_to[2], &raw[2]);

                REQUIRE(result);
            }
            SECTION("gt2")
            {
                bool result = bits::internal::greater_than_le(&raw[0], &raw[2], &compare_to[2]);

                REQUIRE(!result);
            }
            SECTION("gt3")
            {
                uint8_t compare_to2[] { 0x35, 0x12 };

                bool result = bits::internal::greater_than_le(&raw[0], &raw[2], &compare_to2[2]);

                REQUIRE(!result);
            }
            SECTION("lt1")
            {
                bool result = bits::internal::compare_le<false, false>(&raw[0], &raw[2], &compare_to[2]);

                REQUIRE(result);
            }
        }
        SECTION("big endian")
        {
            raw[0] = 0x12;
            raw[1] = 0x34;

            compare_to[0] = 0x13;
            compare_to[1] = 0x34;

            SECTION("gt1")
            {
                using compare = bits::internal::compare<bits::endianness::big_endian, true, false>;

                bool result = compare::eval(&compare_to[0], &compare_to[2], &raw[0]);

                REQUIRE(result);
            }
        }
    }
    SECTION("2nd generation")
    {
        SECTION("width deducer")
        {
            REQUIRE(bits::internal::width_deducer(0x3) == 8);
            REQUIRE(bits::internal::width_deducer(0x13) == 8);
            REQUIRE(bits::internal::width_deducer(0x313) == 16);
            REQUIRE(bits::internal::width_deducer(0x1313) == 16);
            REQUIRE(bits::internal::width_deducer(0x31313) == 24);
            REQUIRE(bits::internal::width_deducer(0x131313) == 24);
            REQUIRE(bits::internal::width_deducer(0x3131313) == 32);

            REQUIRE(bits::internal::width_deducer2(0x3) == 8);
            REQUIRE(bits::internal::width_deducer2(0x13) == 8);
            REQUIRE(bits::internal::width_deducer2(0x313) == 16);
            REQUIRE(bits::internal::width_deducer2(0x1313) == 16);
            REQUIRE(bits::internal::width_deducer2(0x31313) == 24);
            REQUIRE(bits::internal::width_deducer2(0x131313) == 24);
            REQUIRE(bits::internal::width_deducer2(0x3131313) == 32);
        }
        SECTION("little endian")
        {
            typedef bits::internal::getter<uint16_t, bits::little_endian, bits::lsb_to_msb> getter_16;
            typedef bits::internal::getter<uint32_t, bits::little_endian, bits::lsb_to_msb> getter_32;
            typedef bits::internal::setter<uint16_t, bits::little_endian, bits::lsb_to_msb> setter_16;
            typedef bits::internal::setter<uint32_t, bits::little_endian, bits::lsb_to_msb> setter_32;

            SECTION("setter")
            {
                SECTION("16-bit")
                {
                    setter_16::set(bits::descriptor{0, 16}, raw, 0x123);
                }
                SECTION("16-bit bitpos=4, len=7")
                {
                    // direct from [2] 2.1.3.1 example
                    setter_16::set(bits::descriptor{4, 7}, raw, endian_example2_1_3_1);

                    REQUIRE((int)raw[0] == 0b10100000);
                    REQUIRE((int)raw[1] == 0b00000101);
                }
                SECTION("32-bit")
                {
                    setter_32::set(bits::descriptor{0, 32}, raw, endian_example1);
                    compare_le_example1(raw);
                }
                SECTION("32-bit bitpos=4, len=7")
                {
                    setter_32::set(bits::descriptor{4, 7}, raw, endian_example2_1_3_1);

                    test::compare(raw, le_example2_1_3_1, 2);
                }
                SECTION("32-bit bitpos=4, len=15")
                {
                    setter_32::set(bits::descriptor{4, 15}, raw, endian_example2_1_3_1_aug);

                    test::compare(raw, le_example2_1_3_1_aug, 3);
                }
            }
            SECTION("getter")
            {
                SECTION("16-bit")
                {
                    uint16_t v = getter_16::get(16, bits::descriptor{4, 7},
                                                le_example2_1_3_1 + 1);

                    REQUIRE(v == endian_example2_1_3_1);
                }
                SECTION("32-bit")
                {
                    uint32_t v = getter_32::get(32, bits::descriptor{0, 32},
                                                le_example1 + 3);

                    REQUIRE(v == endian_example1);
                }
                SECTION("32-bit bitpos=4, len=7")
                {
                    uint32_t v = getter_32::get(16, bits::descriptor{4, 7},
                                                le_example2_1_3_1 + 1);

                    REQUIRE(v == endian_example2_1_3_1);
                }
                SECTION("32-bit bitpos=4, len=7")
                {
                    uint32_t v = getter_32::get(32, bits::descriptor{4, 7},
                                                le_example2_1_3_1_32bit + 3);

                    REQUIRE(v == endian_example2_1_3_1);
                }
            }
        }
    }
}