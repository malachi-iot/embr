#pragma once

namespace embr {

namespace internal {

/// Helper to forcefully reduce precision on type U to type T
/// @tparam U
/// @tparam T
/// @tparam Enabled
/// @remarks Spiritual relative of https://stackoverflow.com/questions/58615926/what-does-narrow-cast-do
template <class U, class T, typename Enabled = void>
struct narrow_cast;


}

}