
// OTS_v10.pdf [1.2] Section 3.3

#include "fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

struct ObjectActionControlPointBase
{
    enum OpCodes : uint8_t
    {
        CREATE = 1,
        DELETE,
        CHECKSUM,
        EXECUTE,
        READ,
        WRITE,
        ABORT,

        RESPONSE_CODE = 0x60,
    };

    PACK(struct OffsetLength
    {
        uint32 offset;
        uint32 length;
    });

    PACK(struct Create
    {
        uint32 size;
        int type;   // DEBT: placeholder
    });
};

PACK(struct ObjectActionControlPoint : ObjectActionControlPointBase
{
    OpCodes opcode;
    PACK(union
    {
        char parameter[20];
        OffsetLength read;
        OffsetLength write;
        OffsetLength checksum;
    });
});

template <>
struct characteristic_traits<ObjectActionControlPoint>
{
    static constexpr const char* description()
    {
        return "Object Action Control Point";
    }

    static constexpr uint16_t uuid = 0x2AC5;
};

}}}}

