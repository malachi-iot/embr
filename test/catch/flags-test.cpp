#include <catch2/catch_all.hpp>

#include <embr/flags.h>
#include <embr/internal/word/v2/enum.h>


// DEBT: Some duplication with estd::flags testing, but useful to have
// our word options sanity checked too so leaving for now
TEST_CASE("flags (compile-time capable)", "[flags]")
{
    using namespace embr;

    SECTION("~ (not)")
    {
        auto v = ~v2::word_options::native;
        //auto v2 = unsigned(v.value());

        REQUIRE((v & v2::word_options::native) == false);
        REQUIRE((v & v2::word_options::implicit) == true);
    }
    SECTION("xor")
    {
        constexpr auto v1 = v2::word_options::implicit;
        constexpr auto v2 = v1 ^ v2::word_options::implicit;

        REQUIRE(v2 == false);
    }
    SECTION("compare")
    {
        auto v1 = v2::word_options::none | v2::word_options::implicit;
        auto v2 = v2::word_options::none | v2::word_options::implicit;

        bool b = v1 == v2;

        REQUIRE(b);

        auto v3 = v2::word_options::implicit & ~v2::word_options::implicit;

        REQUIRE(v3 == v2::word_options::none);
        REQUIRE(v3 != v1);
    }
}
