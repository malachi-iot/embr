#pragma once

namespace embr { namespace experimental {

// Through lookup mechanism, resolves TOwner + property ID back to property traits
template <class TOwner, int id_>
using PropertyTraits3 = typename TOwner::id::template lookup<id_>;

namespace event {

template <typename T = void, int id = -1, class enabled = void>
struct PropertyChanged;


template <typename T, int id = -1, class enabled = void>
struct PropertyChanging;


}

template <class TImpl, class TSubject>
class PropertyHost;

}}