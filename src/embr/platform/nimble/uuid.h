#pragma once

#include <estd/utility.h>

#include <nimble/ble.h>
#include <host/ble_uuid.h>

#include "../ble/gatt/char/enum.h"
#include "../ble/gatt/enum.h"

namespace embr { namespace nimble { inline namespace v1 {

template <uint16_t uuid>
constexpr const ble_uuid16_t& make_uuid16()
{
    static constexpr ble_uuid16_t v{BLE_UUID_TYPE_16, uuid};
    return v;
}

template <uint32_t uuid>
constexpr const ble_uuid32_t& make_uuid32()
{
    static constexpr ble_uuid32_t v{BLE_UUID_TYPE_32, uuid};
    return v;
}

// EXPERIMENTAL
template <const char (&uuid)[16]>
constexpr const ble_uuid128_t& make_uuid128()
{
    static constexpr ble_uuid128_t v{BLE_UUID_TYPE_128, uuid};
    return v;
}


#if __cplusplus >= 201703L
template <auto uuid>
struct make_uuid;

template <uint16_t uuid>
struct make_uuid<uuid> : estd::integral_constant<const ble_uuid_t*, &make_uuid16<uuid>().u> {};

template <ble::gatt::v1::uuid::Characteristic16 uuid>
struct make_uuid<uuid> : estd::integral_constant<const ble_uuid_t*, &make_uuid16<uuid>().u> {};

template <ble::gatt::v1::service::uuid::Services16 uuid>
struct make_uuid<uuid> : estd::integral_constant<const ble_uuid_t*, &make_uuid16<uuid>().u> {};

template <auto v>
constexpr const ble_uuid_t* uuid = embr::nimble::v1::make_uuid<v>::value;
#endif


}}}
