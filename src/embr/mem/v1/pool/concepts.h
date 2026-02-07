#pragma once

#if __cpp_lib_concepts
#include <concepts>

namespace embr::mem::detail::inline v1::concepts {

template <class T>
concept ContainerProviderTraits = requires
{
    // DEBT: Grab estd Container concept and assert its use here for container_type
    typename T::container_type;
};

template <class T>
concept HandlesTraits = ContainerProviderTraits<T> && requires
{
    typename T::handle_type;
};

}

#endif