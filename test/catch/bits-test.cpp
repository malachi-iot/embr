/**
 * References:
 *
 * 1. https://github.com/malachib/chariot/tree/main/lib/j1939/j1939 v0.1
 * 2. bits/README.md
 */
#include <catch.hpp>

#include <embr/bits/bits.hpp>

#include "test-data.h"

using namespace embr;

void compare_le_example1(const uint8_t raw[])
{
    test::compare(raw, le_example1, 4);
}

template <std::size_t N>
void clear(estd::array<uint8_t, N>& a)
{
    estd::fill(a.begin(), a.end(), 0);
}

template <bits::endianness e>
struct macro_tester_1 : bits::layer1::material<e, 8>
{
    typedef bits::layer1::material<e, 8> base_type;

    EMBR_BITS_ENCODER_SETTER(test2, 0, 0, 4);
    EMBR_BITS_DECODER_GETTER(test2, 0, 0, 4);

    EMBR_BITS_MATERIAL_PROPERTY(test3, 0, 0, 4);
    EMBR_BITS_MATERIAL_PROPERTY(test4, 0, 4, 4);
    EMBR_BITS_MATERIAL_PROPERTY(test5, 1, 0, 9);

    void set_test1(unsigned v)
    {
        base_type::template set<0, 4>(0, v);
    }

    embr::word<4> get_test1() const
    {
        return base_type::template get<0, 4>(0);
    }
};

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
                    bits::get<bits::no_endian, uint8_t, bits::lsb_to_msb>(
                        bits::descriptor{ 0, 4}, be_example1);
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

            bits::layer2::material<bits::big_endian, sizeof(be_example1)> item{raw};

            REQUIRE(!(item < item));
            REQUIRE(!(item > item));
            REQUIRE((item == item));

            SECTION("3rd generation")
            {
                // auto-converting up to 16 bit for convenience
                embr::word<16> v = item.get<4, 4>(0);

                REQUIRE(v == 1);

                // FIX: word<8> overflows on shift operations while this
                // is crossing boundaries
                //v = item.get<4, 8>(1);

                //REQUIRE(v.value() == 0x35);
            }
        }
        SECTION("little endian")
        {
            std::copy_n(le_example1, sizeof(le_example1), raw);

            bits::layer2::material<bits::little_endian, 4> item{raw};

            REQUIRE(!(item < item));
            REQUIRE(!(item > item));
            REQUIRE((item == item));
        }
    }
    SECTION("layer1 operations")
    {
        SECTION("encoder")
        {
            bits::layer1::encoder<bits::big_endian, 4, bits::lsb_to_msb, bits::msb_to_lsb> e;

            clear(e);

            e.set<uint8_t>(0, bits::descriptor{4, 3}, 3);

            REQUIRE(e.data()[0] == 0x30);
        }
        SECTION("material")
        {
            SECTION("big endian")
            {
                bits::layer1::material<bits::big_endian, 4, bits::lsb_to_msb> e;

                clear(e);

                e.set<uint8_t>(0, bits::descriptor{4, 3}, 3);

                REQUIRE(e.data()[0] == 0x30);
            }
            SECTION("little endian")
            {
                // It seems le_example1's constness is not compatible with estd::array ctor
                /*
                bits::layer1::material<bits::little_endian, 4> item{le_example1};

                REQUIRE(!(item < item));
                REQUIRE(!(item > item));
                REQUIRE((item == item)); */
            }
        }
    }
    SECTION("bits::word")
    {
        constexpr int v1 = 5;
        constexpr int v2 = 10;
        constexpr bool v3 = true;

        SECTION("General")
        {
            bits::internal::word<16> w;
        }
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

            REQUIRE(v.get_exp<traits_type>() == v1);

            REQUIRE(d == traits_type::get_descriptor());
        }
    }
    SECTION("v3")
    {
        SECTION("getters/setters")
        {
            using namespace bits;

            // TODO: Needs more work

            SECTION("big_endian, lsb_to_msb, 8-bit")
            {
                uint8_t v;

                SECTION("4, 8")
                {

                    typedef detail::getter<4, 8, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{4, 8};

                    g::get(d, be_example1, v);

                    REQUIRE(v == 0x14);
                }
                SECTION("2, 7")
                {
                    typedef detail::getter<2, 7, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{2, 7};

                    g::get(d, &be_example1[0], v);

                    // 0x12 0x34 0x56 = 0b00010010 0b00110100 0b01010110
                    // making result 0b000100xx + 0b.......0 = 0b00001000 = 0x08

                    REQUIRE(v == 0x08);
                }
                SECTION("subbyte 1, 3")
                {
                    typedef detail::getter<1, 3, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{1, 3};

                    //g::get_unready(d, &be_example1[0], v);

                    // FIX: wire in subbyte here - probably located in embr::bits or embr::bits::internal
                    // remember, subbyte is not multibyte so endianness does not apply
                }
            }
            SECTION("big_endian, lsb_to_msb, 16-bit")
            {
                uint16_t v;

                SECTION("4, 16")
                {
                    typedef detail::getter<4, 16, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{4, 16};

                    g::get(d, be_example1, v);

                    REQUIRE(v == 0x1346);
                }
                SECTION("2, 15")
                {
                    typedef detail::getter<4, 15, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{2, 15};

                    g::get(d, be_example1, v);

                    // 0x12 0x34 0x56 = 0b00010010 0b00110100 0b01010110
                    // 0b000100xx 0b00110100 0b.......0 = 0b00001000 0b01101000 = 0x868

                    REQUIRE(v == 0x868);
                }
                // Byte boundary flavor
                SECTION("0, 16")
                {
                    typedef detail::getter<0, 16, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{0, 16};

                    g::get(d, be_example1, v);

                    REQUIRE(v == 0x1234);
                }
            }
            SECTION("big_endian, lsb_to_msb, 32-bit")
            {
                uint32_t v;

                SECTION("4, 24")
                {
                    typedef detail::getter<4, 24, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{4, 24};

                    g::get(d, be_example1, v);

                    REQUIRE(v == 0x134568);
                }
                SECTION("2, 24")
                {
                    typedef detail::getter<2, 24, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{2, 24};

                    g::get(d, be_example1, v);

                    // 0x12 0x34 0x56 = 0b00010010 0b00110100 0b01010110
                    // 0b000100xx 0b00110100 0b01010110 0b......00 = 0b00010000 0b11010001 0b01011000

                    REQUIRE(v == 0x10D158);
                }
                SECTION("3, 27")
                {
                    typedef detail::getter<3, 27, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{3, 27};

                    g::get(d, be_example1, v);

                    // 0x12 0x34 0x56 0x78 = 0b00010010 0b00110100 0b01010110 0b01111000
                    // 0b00010xxx 0b00110100 0b01010110 0b..111000 = 000 10001101 00010101 10111000

                    REQUIRE(v == 0x8D15B8);
                }
                // Byte boundary flavor
                SECTION("0, 24")
                {
                    typedef detail::getter<0, 24, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{0, 24};

                    g::get(d, be_example1, v);

                    REQUIRE(v == 0x123456);
                }
                // Byte boundary flavor
                SECTION("0, 32")
                {
                    typedef detail::getter<0, 32, big_endian, lsb_to_msb> g;
                    constexpr descriptor d{0, 32};

                    g::get(d, be_example1, v);

                    REQUIRE(v == 0x12345678);
                }
            }

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
            typedef bits::getter<bits::little_endian, bits::lsb_to_msb> getter;
            typedef bits::setter<bits::little_endian, bits::lsb_to_msb> setter;

            SECTION("setter")
            {
                SECTION("16-bit")
                {
                    setter::set(bits::descriptor{0, 16}, raw, 0x123);
                }
                SECTION("16-bit bitpos=4, len=7")
                {
                    // direct from [2] 2.1.3.1 example
                    setter::set(bits::descriptor{4, 7}, raw, endian_example2_1_3_1);

                    REQUIRE((int)raw[0] == 0b10100000);
                    REQUIRE((int)raw[1] == 0b00000101);
                }
                SECTION("32-bit")
                {
                    setter::set(bits::descriptor{0, 32}, raw, endian_example1);
                    compare_le_example1(raw);
                }
                SECTION("32-bit bitpos=4, len=7")
                {
                    setter::set(bits::descriptor{4, 7}, raw, endian_example2_1_3_1);

                    test::compare(raw, le_example2_1_3_1, 2);
                }
                SECTION("32-bit bitpos=4, len=15")
                {
                    setter::set(bits::descriptor{4, 15}, raw, endian_example2_1_3_1_aug);

                    test::compare(raw, le_example2_1_3_1_aug, 3);
                }
            }
            SECTION("getter")
            {
                SECTION("16-bit")
                {
                    uint16_t v = getter::get<uint16_t>(bits::descriptor{4, 7},
                                                le_example2_1_3_1 + 1, false);

                    REQUIRE(v == endian_example2_1_3_1);
                }
                SECTION("32-bit")
                {
                    uint32_t v = getter::get<uint32_t>(bits::descriptor{0, 32},
                                                le_example1 + 3, false);

                    REQUIRE(v == endian_example1);
                }
                SECTION("32-bit bitpos=4, len=7")
                {
                    uint32_t v = getter::get<uint32_t>(bits::descriptor{4, 7},
                                                le_example2_1_3_1 + 1, false);

                    REQUIRE(v == endian_example2_1_3_1);
                }
                SECTION("32-bit bitpos=4, len=7")
                {
                    uint32_t v = getter::get<uint32_t>(
                                                //bits::descriptor{4, 7},
                                                bits::descriptor{4, 7 + 16},    // width deducer needs extra 16 bits here
                                                le_example2_1_3_1_32bit + 3,
                                                false);

                    REQUIRE(v == endian_example2_1_3_1);
                }
            }
        }
    }
    // Adapted from 2nd Generation tests
    SECTION("3rd Generation")
    {
        SECTION("little endian")
        {
            typedef bits::detail::setter<0, 16, bits::little_endian, bits::lsb_to_msb> setter_16_bb;
            typedef bits::detail::setter<1, 14, bits::little_endian, bits::lsb_to_msb> setter_16;

            // Remember, v3 setter and getter are resilient to underlying type bitness
            typedef setter_16_bb setter_bb;
            typedef setter_16 setter_full;
            //typedef bits::experimental::setter<0, 32, bits::little_endian, bits::lsb_to_msb> setter_32_bb;
            //typedef bits::experimental::setter<1, 30, bits::little_endian, bits::lsb_to_msb> setter_32;

            SECTION("get assister")
            {
                // NOTE: Not convinced we really want a distinct get_assister.  See comments
                // on struct itself
                int i;
                const uint8_t* raw = le_example1;
                uint8_t v8;
                uint16_t v16;
                uint32_t v32;

                typedef bits::internal::get_assister<bits::little_endian, true, uint8_t> ga_8;
                typedef bits::internal::get_assister<bits::little_endian, true, uint16_t> ga_16;
                typedef bits::internal::get_assister<bits::little_endian, true, uint32_t> ga_32;

                /*
                //ga_8::get_assist(i = 0, raw, v8);
                ga_16::get_assist(i = 16, raw, v16);
                ga_32::get_assist(i = 32, raw, v32);

                REQUIRE(v32 == endian_example1); */
            }
            SECTION("setter")
            {
                SECTION("16-bit")
                {
                    setter_16_bb::set(bits::descriptor{0, 16}, raw, 0x123);

                    REQUIRE((int)raw[0] == 0x23);
                    REQUIRE((int)raw[1] == 0x01);
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
                    setter_bb::set(bits::descriptor{0, 32}, raw, endian_example1);
                    compare_le_example1(raw);
                }
                SECTION("32-bit bitpos=4, len=7")
                {
                    setter_full::set(bits::descriptor{4, 7}, raw, endian_example2_1_3_1);

                    test::compare(raw, le_example2_1_3_1, 2);
                }
                SECTION("32-bit bitpos=4, len=15")
                {
                    setter_full::set(bits::descriptor{4, 15}, raw, endian_example2_1_3_1_aug);

                    test::compare(raw, le_example2_1_3_1_aug, 3);
                }
            }
            SECTION("getter")
            {
                typedef bits::detail::getter<0, 16, bits::little_endian, bits::lsb_to_msb> getter_bb;
                typedef bits::detail::getter<1, 14, bits::little_endian, bits::lsb_to_msb> getter_full;

                SECTION("16-bit")
                {
                    uint16_t v;
                    
                    getter_full::get(bits::descriptor{4, 7},
                                                le_example2_1_3_1 + 1, v);

                    REQUIRE(v == endian_example2_1_3_1);
                }
                SECTION("16-bit")
                {
                    uint16_t v;

                    bits::descriptor d{4, 7};
                    int adjuster = getter_full::adjuster(d);

                    getter_full::get(d,
                                     le_example2_1_3_1 + adjuster, v);

                    REQUIRE(v == endian_example2_1_3_1);
                }
                SECTION("32-bit")
                {
                    uint32_t v;

                    bits::descriptor d{0, 32};
                    int adjuster = getter_bb::adjuster(d);

                    getter_bb::get(d, le_example1 + adjuster, v);

                    REQUIRE(v == endian_example1);
                }
            }
        }
        SECTION("big endian")
        {
            SECTION("setter")
            {
                typedef bits::detail::setter<0, 16, bits::big_endian, bits::lsb_to_msb> setter_bb;
                typedef bits::detail::setter<1, 14, bits::big_endian, bits::lsb_to_msb> setter_full;
            }
            SECTION("getter")
            {
                typedef bits::detail::getter<0, 16, bits::big_endian, bits::lsb_to_msb> getter_bb;
                typedef bits::detail::getter<1, 14, bits::big_endian, bits::lsb_to_msb> getter_full;

                SECTION("16-bit")
                {
                    uint16_t v;

                    bits::descriptor d{4, 7};
                    int adjuster = getter_full::adjuster(d);

                    getter_full::get(d,
                                     be_example2_1_1 + adjuster, v);

                    // FIX: Appears to have an msb/lsb resume glitch
                    //REQUIRE(v == endian_example2_1_1);
                }
                SECTION("32-bit")
                {
                    uint32_t v;

                    bits::descriptor d{0, 32};
                    int adjuster = getter_bb::adjuster(d);

                    getter_bb::get(d, be_example1 + adjuster, v);

                    REQUIRE(v == endian_example1);
                }
            }
        }
    }
}

using namespace embr::bits;

template <class TInt, length_direction direction>
static void tester(uint8_t* buf, TInt compare_to, uint8_t bitpos, uint8_t len)
{
    // DEBT: Endian doesn't matter much here as this is used for byte-only tests
    typedef bits::getter<little_endian, direction> getter;

    --bitpos;
    //--len;

    INFO("bitpos = " << (int)bitpos << ", len = " << (int)len);

    TInt v = getter::template get<TInt>(descriptor{bitpos, len}, buf);
    //TInt v = bit_get<TInt, direction>(bit_descriptor{1, bitpos, len }, buf);

    REQUIRE(compare_to == v);
}

// DEBT: Two different TEST_CASE in one file, temporary - artifact of importing code
// from 'chariot'.  Consolidate and organize - perhaps there's so many bit tests two
// different TEST_CASE are still warranted, but even if so, them named 'bits' and 'bits2'
// and in the same file is not helpful
TEST_CASE("bits")
{
    uint8_t* buf1 = test_data::buf1;

    SECTION("uint8_t lsb_to_msb")
    {
        uint8_t _buf = *buf1;
        tester<uint8_t, length_direction::lsb_to_msb>(buf1, _buf & 0b11, 1, 2);
        _buf >>= 2;
        tester<uint8_t, length_direction::lsb_to_msb>(buf1, _buf & 0b11, 3, 2);
        _buf >>= 2;
        tester<uint8_t, length_direction::lsb_to_msb>(buf1, _buf & 0b11, 5, 2);
        _buf >>= 2;
        tester<uint8_t, length_direction::lsb_to_msb>(buf1, _buf & 0b11, 7, 2);
    }
    SECTION("uint8_t msb_to_lsb")
    {
        uint8_t _buf = *buf1;
        tester<uint8_t, length_direction::msb_to_lsb>(buf1, _buf & 0b11, 2, 2);
        _buf >>= 2;
        tester<uint8_t, length_direction::msb_to_lsb>(buf1, _buf & 0b11, 4, 2);
        _buf >>= 2;
        tester<uint8_t, length_direction::msb_to_lsb>(buf1, _buf & 0b11, 6, 2);
        _buf >>= 2;
        tester<uint8_t, length_direction::msb_to_lsb>(buf1, _buf & 0b11, 8, 2);
    }
    SECTION("bool")
    {
        //typedef bits::internal::getter<bool, little_endian, lsb_to_msb> getter;
        // DEBT: No bool specialization just yet
        typedef bits::getter<little_endian, lsb_to_msb> getter;

        const uint8_t* buf = buf1;
        uint8_t _buf = *buf;

        for(uint8_t i = 1; i <= 8; ++i)
        {
            INFO("bitpos = " << (int)(i-1));

            //bool v = bit_get<bool, length_direction::lsb_to_msb>(bit_descriptor{1, i }, buf);
            bool v = getter::template get<byte>(descriptor{ i - 1, 1 }, buf);

            REQUIRE((_buf & 1) == v);

            _buf >>= 1;
        }
    }
    SECTION("big endian (msb_to_lsb)")
    {
        typedef bits::getter<big_endian, msb_to_lsb> getter;

        SECTION("bool")
        {
            //bool v = bits_type::get<bool>(bit_descriptor{1, 1}, buf1);
        }
        SECTION("uint16_t")
        {
            //auto v = bits_type::get<uint16_t>(bit_descriptor{1, 1, 9}, buf1);
            auto v = getter::get<uint16_t>(descriptor{0, 9}, buf1);

            REQUIRE(v == 0x181);
        }
        SECTION("9-bit length, bitpos=3")
        {
            /// +++ "BAD" version, maybe I was thinking of lsb ordered bits?
            // bitpos=4 (1-based) in MSB bit order, that would turn this:
            // { 0b10111111, 0b10000001 } // 0xBF 0x81
            // into
            // { 0b00010111, 0b10000001 } // 0x17 0x81
            // then the 9 bit length mask would turn it into:
            // { 0b00010111, 0b10000000 } // 0x17 0x80
            // FIX: Mask only needs to filter MSB's out.  LSB's need to be bit shifted away
            // (i.e. we actually need { 0b00000001, 0b01111000 } 0x178
            /// ---
            // bitpos=3 (1-based) in MSB bit order, that would turn this:
            // { 0b10111111, 0b10000001 } // 0xBF 0x81
            // into
            // { 0b00000111, 0b10000001 } // 0x07 0x81
            //          987    654321
            // then shifting according to 9 bit length would turn that into
            // { 0b00000001, 0b11100000 } // 0x01 0xE0
            //            9    87654321
            //auto v = bits_type::get<uint16_t>(bit_descriptor{1, 3, 9}, buf1);
            auto v = getter::get<uint16_t>(descriptor{2, 9}, buf1);

            REQUIRE(v == 0x1E0);
        }
        SECTION("uint32_t")
        {
            //auto v = bits_type::get<uint32_t>(bit_descriptor{1, 8, 32}, be_example1);
            auto v = getter::get<uint32_t>(descriptor{7, 32}, be_example1);

            REQUIRE(v == endian_example1);
        }
    }
    SECTION("big endian (lsb to msb)")
    {
        typedef bits::getter<big_endian, lsb_to_msb> getter;
        //typedef bits<endianness::big_endian, length_direction::lsb_to_msb> bits_type;

        SECTION("uint16_t")
        {
            // { 0b10111111, 0b10000001 } // 0xBF 0x81
            // becomes
            // { 0b10111111, 0b1xxxxxxx } // 0xBF 0x80
            // becomes
            // { 0b00000001, 0b01111111 } // 0x01 0x7F
            //auto v = bits_type::get<uint16_t>(bit_descriptor{1, 1, 9}, buf1);
            auto v = getter::get<uint16_t>(descriptor{0, 9}, buf1);

            REQUIRE(v == 0x17F);
        }
        SECTION("uint16_t, bitpos=4, length=9")
        {
            // { 0b10111111, 0b10000001 } // 0xBF 0x81
            // becomes
            // { 0b10111xxx, 0b1000xxxx } // 0xB? 0x8? aka 0x17 0x8
            // becomes
            // { 0b00000001, 0b01111000 } // 0x01 0x78
            //auto v = bits_type::get<uint16_t>(bit_descriptor{1, 4, 9}, buf1);
            auto v = getter::get<uint16_t>(descriptor{3, 9}, buf1);

            REQUIRE(v == 0x178);
        }
        SECTION("uint16_t, bitpos=5, length=7")
        {
            // { 0b10111111, 0b10000001 } // 0xBF 0x81
            // becomes
            // { 0b1011xxxx, 0b100xxxxx } // aka 0xB 0x4
            // becomes
            // { 0b00000000, 0b01011100 } // 0x00 0x5C
            //auto v = bits_type::get<uint16_t>(bit_descriptor{1, 5, 7}, buf1);
            auto v = getter::get<uint16_t>(descriptor{4, 7}, buf1);

            REQUIRE(v == 0x5C);
        }
        SECTION("uint32_t")
        {
            //auto v = bits_type::get<uint32_t>(bit_descriptor{1, 1, 32}, be_example1);
            auto v = getter::get<uint32_t>(descriptor{0, 32}, be_example1);

            REQUIRE(v == endian_example1);
        }
        SECTION("uint32_t, bitpos=5, length=20")
        {
            // { 0x12, 0x34, 0x56, 0x78} ->
            // { 0x01, 0x34, 0x56 }
            //auto v = bits_type::get<uint32_t>(bit_descriptor{1, 5, 20}, be_example1);
            auto v = getter::get<uint32_t>(descriptor{4, 20}, be_example1);

            REQUIRE(v == 0x13456);
        }
    }
    SECTION("macro helpers")
    {
        SECTION("EMBR_BITS_ENCODER_SETTER + EMBR_BITS_ENCODER_GETTER")
        {
            macro_tester_1<bits::little_endian> v;

            v.fill(0);

            v.set_test1(10);

            REQUIRE(v.get_test1() == 10);

            v.test2(15);

            REQUIRE(v.test2() == 15);
            REQUIRE(v.test3() == 15);

            v.test4(1);
            v.test5(0x123);

            REQUIRE(v.test3() == 15);
            REQUIRE(v.test4() == 1);
            REQUIRE(v.test5() == 0x123);
        }
    }
#if TO_MIGRATE
    SECTION("little endian (msb to lsb)")
    {
        typedef bits::internal::getter<uint16_t, little_endian, msb_to_lsb> getter;
        typedef bits::internal::getter<uint32_t, little_endian, msb_to_lsb> getter_32;
        // NOTE: All the little endian stuff may be slightly off.  'bitpos' actually should be
        // calculated against the last byte, not the first byte, in a little endian scenario
        //typedef bits<endianness::little_endian, length_direction::msb_to_lsb> bits_type;

        SECTION("bool")
        {
            //bool v = bits_type::get<bool>(bit_descriptor{1, 1}, buf1);
        }
        SECTION("uint16_t")
        {
            SECTION("16-bit length")
            {
                //auto v = bits_type::get<uint16_t>(bit_descriptor{1, 8, 16}, buf1);
                auto v = getter::get_adjusted(descriptor{7, 16}, buf1);

                REQUIRE(v == 0x81BF);
            }
            SECTION("9-bit length")
            {
                // { 0b10111111, 0b10000001 } 0xBF 81 -> bitpos + length filtering
                // { 0bxxxxxxx1, 0b10000001 } 0x1 81 -> swapping
                // { 0b10000001, 0bxxxxxxx1 } 0x81 0x1 -> suturing
                // { 0b00000001, 0b00000011 } 0x1 0x3
                auto v = bits_type::get<uint16_t>(bit_descriptor{1, 1, 9}, buf1);

                //REQUIRE(v == 0x103);
            }
            SECTION("8-bit length, bitpos=3")
            {
                // { 0b10111111, 0b10000001 } 0xBF 81 -> bitpos + length filtering
                // { 0bxxxxx111, 0bxxx00001 } 0x3 0x01 -> swapping
                // { 0bxxx00001, 0bxxxxx111 } 0x1 0x03 -> suturing
                // { 0b00000000, 0b00001111 } 0x0 0xF
                auto v = bits_type::get<uint16_t>(bit_descriptor{1, 3, 8}, buf1);

                //REQUIRE(v == 0xF);
            }
        }
        SECTION("uint32_t")
        {
            SECTION("32-bit length")
            {
                auto v = bits_type::get<uint32_t>(bit_descriptor{1, 8, 32}, le_example1);

                REQUIRE(v == endian_example1);
            }
            SECTION("24-bit length, bytepos=2")
            {
                // TODO: Need to bounds check on 32-bit, as there may not actually
                // be a full 32 bit word available in the raw data stream
                //auto v = bits_type::get<uint32_t>(bit_descriptor{2, 8, 24}, le_example1);

                //REQUIRE(v == endian_example1);
            }
        }
    }
#endif // TO_MIGRATE
}
