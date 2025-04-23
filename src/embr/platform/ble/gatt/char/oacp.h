#pragma once

// OTS_v10.pdf [1.2] Section 3.3

#include "fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

struct ObjectActionControlPointBase
{
    enum Opcodes : uint8_t
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

    enum ResultCodes : uint8_t
    {
        SUCCESS = 1,
        UNSUPPORTED_OPCODE,
        INVALID_PARAMETER,
        INSUFFICIENT_RESOURCES,
        INVALID_OBJECT,
        CHANNEL_UNAVAILABLE,
        UNSUPPORTED_TYPE,
        NOT_PERMITTED,
        OBJECT_LOCKED,
        FAILED
    };

    PACK(struct Response
    {
        Opcodes opcode;
        ResultCodes result_code;
        // parameter
        PACK(union
        {
            uint32 checksum;
        });
    });
};

PACK(struct ObjectActionControlPoint : ObjectActionControlPointBase
{
    Opcodes opcode;

    // DEBT: Technically only gcc really plays nice here due to type-punning
    PACK(union
    {
        char parameter[20];
        Create create;
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

    static constexpr v1::uuid::Characteristic16 uuid = v1::uuid::ObjectActionControlPoint;
};

}}}}

