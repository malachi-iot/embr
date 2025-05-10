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

    enum ListSortOrders : uint8_t
    {
        BY_NAME = 1,
        BY_TYPE,
        BY_SIZE,
        BY_CREATED,
        BY_MODIFIED,

        // OR mask
        ASCENDING = 0,
        DESCENDING = 0x10,
    };

    enum ResultCodes : uint8_t
    {
        SUCCESS = 1,
        UNSUPPORTED_OPCODE,
        INVALID_PARAMETER,
        FAILED,
        OUT_OF_BOUNDS,
        TOO_MANY_OBJECTS,
        NO_OBJECT
    };

    PACK(struct Response
    {
        Opcodes opcode;
        ResultCodes result_code;
        uint32 number_of_objects;
    });
};

PACK(struct ObjectListControlPoint : ObjectListControlPointBase
{
    Opcodes opcode;
    PACK(union
    {
        uint8_t parameter[6];
        Response response;
        uint48 goto_object_id;
        ListSortOrders order;
    });
});

template <>
struct characteristic_traits<ObjectListControlPoint>
{
    static constexpr const char* description()
    {
        return "Object List Control Point";
    }

    static constexpr v1::uuid::Characteristic16 uuid = v1::uuid::ObjectListControlPoint;;
};


}

}}}
