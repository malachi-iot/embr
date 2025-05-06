#pragma once

// OTS_v10.pdf [1.2] Section 3.4

#include "fwd.h"
#include "../int.h"

#include "date-time.h"

namespace embr { namespace ble { namespace gatt {

inline namespace v1 {

struct ObjectListFilterBase
{
    enum Filters : uint8_t
    {
        NONE,
        NAME_STARTS_WITH,
        NAME_ENDS_WITH,
        NAME_CONTAINS,
        NAME_IS_EXACTLY,
        OBJECT_TYPE, // UUID
        CREATED_BETWEEN,
        MODIFIED_BETWEEN,
        CURRENT_SIZE_BETWEEN,
        ALLOCATED_SIZE_BETWEEN,
        MARKED
    };

    // "same format as the Date Time characteristic" (OTSv10.pdf Table 3.28)
    PACK(struct TimeBetween
    {
        DateTime timestamp1;
        DateTime timestamp2;
    });

    PACK(struct SizeBetween
    {
        uint32 size1;
        uint32 size2;
    });
};

PACK(struct ObjectListFilter : ObjectListFilterBase
{
    Filters filter;
    PACK(union
    {
        TimeBetween time_between;
        SizeBetween size_between;
    });
});
    
}

template <>
struct characteristic_traits<v1::ObjectListFilter>
{
    static constexpr const char* description()
    {
        return "Object List Filter";
    }

    static constexpr auto uuid = gatt::v1::uuid::Characteristic16::ObjectListFilter;
};


}}}
