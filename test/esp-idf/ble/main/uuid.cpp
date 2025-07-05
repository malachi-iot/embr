#include <embr/platform/nimble/uuid.h>

using namespace embr;
using namespace embr::ble::gatt::v1;


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
}