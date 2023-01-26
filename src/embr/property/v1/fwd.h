#pragma once

namespace embr {

inline namespace property { inline namespace v1 {

namespace tag {

struct property_traits {};

struct property_owner {};

struct has_id {};

}

namespace event {

template <typename T = void, int id = -1, class enabled = void>
struct PropertyChanged;


template <typename T, int id = -1, class enabled = void>
struct PropertyChanging;


}

template <class TImpl, class TSubject>
class PropertyHost;


}}

}