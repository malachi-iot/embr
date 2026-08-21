#pragma once

namespace embr { namespace internal {

// UNUSED, UNTESTED
template <class T, class String>
struct breadcrumb_base
{
    static constexpr T null_id = (T)-1;

    const String name{};
    const T id = null_id;
    const T parent = null_id;
};

// UNUSED, UNTESTED
using breadcrumb_small = breadcrumb_base<uint8_t, const char*>;

struct breadcrumb
{
    static constexpr int16_t null_id = -1;

    // 21AUG26 MB DEBT: string_view is convenient but ultimately may be a space waster.  json
    // parser seems to favor that fixed size awareness
    const estd::string_view name{};
    const int16_t id = null_id;
    const int16_t parent = null_id;

    static ESTD_CPP_CONSTEVAL breadcrumb null()
    {
        return {  };
    }

    // EXPERIMENTAL - favor using traits::equals instead
    // Since we're performance oriented, do not compare name or parent, only id which is
    // presumed unique
    constexpr bool operator ==(const breadcrumb& compare_to) const
    {
        // TODO: Do asserts on name and parent IF id matches just for integrity checks, but not
        // as an actual == feature

        return id == compare_to.id;
    }

    constexpr bool operator !=(const breadcrumb& compare_to) const
    {
        return operator==(compare_to) == false;
    }
};

}}