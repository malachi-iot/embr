#pragma once

// OTS_v10.pdf [1.2] Section 3.4

#include "fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt {

inline namespace v1 {

struct ObjectListControlPointBase
{
    enum Opcodes : uint8_t
    {
        FIRST = 1,
        LAST,
        PREVIOUS,
        NEXT,
        GOTO,
        ORDER,
        REQUEST_NUM_OBJECTS,
        CLEAR_MARKING,

        RESPONSE_CODE = 0x70,
    };
};

PACK(struct ObjectListControlPoint : ObjectListControlPointBase
{
    Opcodes opcode;
    uint8_t parameter[6];
});

template <>
struct characteristic_traits<ObjectListControlPoint>
{
    static constexpr const char* description()
    {
        return "Object List Control Point";
    }

    static constexpr v1::uuid::Characteristic16 uuid = v1::uuid::ObjectActionControlPoint;;
};


}

}}}
