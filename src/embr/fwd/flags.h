#pragma once

namespace embr { namespace experimental {

template <class Enum>
class flags;

template <class Enum>
constexpr flags<Enum> operator|(const flags<Enum>& lhs, const Enum& rhs);

}}
