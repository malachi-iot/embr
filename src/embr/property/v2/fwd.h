#pragma once

namespace embr { inline namespace property { inline namespace v2 {

template <class Pool, Pool* pool = nullptr>
class provider;

template <const char*>
struct traits;

template <const char* = nullptr, class Enabled = void>
struct changed;

}}}
