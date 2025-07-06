#include <embr/platform/nimble/uuid.h>

using namespace embr;
using namespace embr::ble::gatt::v1;

// AI generated, why not
constexpr const uint8_t uuid_object_transfer[16] = {0x25, 0x18, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB, 0x00, 0x00};


// DEBT: Placement in unity test would be nice, but would require nimble dependency there
void do_uuid()
{
    [[maybe_unused]]
    constexpr const ble_uuid_t* uuid1 = nimble::uuid<uuid::ObjectTransfer>;
    [[maybe_unused]]
    constexpr const ble_uuid_t* uuid2 = nimble::uuid<uuid::ObjectName>;
    [[maybe_unused]]
    constexpr const ble_uuid_t* uuid3 = &nimble::make_uuid16<(uint16_t)0x1234>::v.u;
    [[maybe_unused]]
    constexpr const ble_uuid_t* uuid4 = nimble::make_uuid<uuid::ObjectName>::value;

    // Not quite
    //[[maybe_unused]]
    //constexpr const ble_uuid_t* uuid5 = nimble::make_uuid<uuid_object_transfer>::value;

    [[maybe_unused]]
    constexpr const ble_uuid_t* uuid6 = nimble::uuid16_v<0x1234>;
}