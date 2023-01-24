#pragma once

namespace embr { namespace experimental {

// Through lookup mechanism, resolves TOwner + property ID back to property traits
template <class TOwner, int id_>
using PropertyTraits3 = typename TOwner::id::template lookup<id_>;

namespace event {

// TODO: Consider changing this to property_traits_tag, although
// if under 'properties' namespace perhaps not needed
struct traits_tag
{
};

// TODO: Consider changing this to properties_tag
struct lookup_tag
{
};

template <typename T = void, int id = -1, class enabled = void>
struct PropertyChanged;


template <typename T, int id = -1, class enabled = void>
struct PropertyChanging;


}

template <class TImpl, class TSubject>
class PropertyHost;

}}