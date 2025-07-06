#pragma once

#include <estd/utility.h>
#include <estd/type_traits.h>

#include <nimble/ble.h>
#include <host/ble_uuid.h>

#include "../ble/gatt/char/enum.h"
#include "../ble/gatt/enum.h"

namespace embr { namespace nimble { inline namespace v1 {

template <const ble_uuid_t* uuid>
using uuid_constant = estd::integral_constant<const ble_uuid_t*, uuid>;

template <uint16_t uuid>
struct make_uuid16
{
    static constexpr ble_uuid16_t v{BLE_UUID_TYPE_16, uuid};
    static constexpr const ble_uuid_t* u = &v.u;
};

template <uint16_t uuid>
constexpr const ble_uuid_t* uuid16_v = make_uuid16<uuid>::u;

template <uint32_t uuid>
struct make_uuid32
{
    static constexpr ble_uuid32_t v{BLE_UUID_TYPE_32, uuid};
    static constexpr const ble_uuid_t* u = &v.u;
};

template <uint32_t uuid>
constexpr const ble_uuid_t* uuid32_v = make_uuid32<uuid>::u;

// EXPERIMENTAL
template <const uint8_t (&uuid)[16]>
struct make_uuid128
{
    static constexpr ble_uuid128_t v{BLE_UUID_TYPE_128, uuid};
    static constexpr const ble_uuid_t* u = &v.u;
};


#if __cplusplus >= 201703L
template <auto uuid>
struct make_uuid;

template <auto v>
constexpr const ble_uuid_t* uuid = embr::nimble::v1::make_uuid<v>::value;

// EXPERIMENTAL
template <const uint8_t (&uuid)[16]>
struct make_uuid<uuid>
{
    static constexpr const ble_uuid_t* value = make_uuid128<uuid>::u;
};


#if __cplusplus >= 202002L
// EXPERIMENTAL
/*
template <const char (&uuid)[16]>
constexpr const ble_uuid128_t& make_uuid128()
{
    static constexpr ble_uuid128_t v{BLE_UUID_TYPE_128, uuid};
    return v;
}   */


template <uint16_t uuid>
struct make_uuid<uuid> : uuid_constant<make_uuid16<uuid>::u> {};

template <ble::gatt::v1::uuid::Characteristic16 uuid>
struct make_uuid<uuid> : uuid_constant<make_uuid16<uuid>::u> {};

template <ble::gatt::v1::service::uuid::Services16 uuid>
struct make_uuid<uuid> : uuid_constant<make_uuid16<uuid>::u> {};

#else
// c++17 mode
template <ble::gatt::v1::uuid::Characteristic16 uuid>
struct make_uuid<uuid>
{
    static constexpr const ble_uuid_t* value = make_uuid16<uuid>::u;
};

template <ble::gatt::v1::service::uuid::Services16 uuid>
struct make_uuid<uuid>
{
    static constexpr const ble_uuid_t* value = make_uuid16<uuid>::u;
};

template <uint16_t uuid>
struct make_uuid<uuid>
{
    static constexpr const ble_uuid_t* value = make_uuid16<uuid>::u;
};


// FIX: Somehow none of this works in c++17 mode.  Have to do above manual flavor
/*
template <uint16_t uuid>
struct make_uuid<uuid> : estd::integral_constant<const ble_uuid_t*,
    (const ble_uuid_t*)&make_uuid16<uuid>::v> {};
*/
/*
template <ble::gatt::v1::uuid::Characteristic16 uuid>
struct make_uuid<uuid> : estd::integral_constant<const ble_uuid_t*,
    make_uuid16<(uint16_t)uuid>::u> {};

template <ble::gatt::v1::service::uuid::Services16 uuid>
struct make_uuid<uuid> : estd::integral_constant<const ble_uuid_t*,
    make_uuid16<(uint16_t)uuid>::u> {};
*/
#endif
#endif

}}}
