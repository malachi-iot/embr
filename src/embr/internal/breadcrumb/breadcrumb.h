#pragma once

namespace embr { namespace internal {

struct breadcrumb
{
    static constexpr int null_id = -1;

    // 19AUG26 MB DEBT: string_view is convenient but ultimately may be a space waster
    const estd::string_view name;
    const int id = null_id;
    const int parent = null_id;

    static ESTD_CPP_CONSTEVAL breadcrumb null()
    {
        return { {} };
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