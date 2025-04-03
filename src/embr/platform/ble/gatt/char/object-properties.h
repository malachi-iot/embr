// OTS_v10.pdf [1.2] Section 3.4

#include "../fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt {

inline namespace v1 {

struct ObjectProperties
{
    // bitmask of permissible operations
    enum Properties
    {
        DELETE      = 1 << 0,
        EXECUTE     = 1 << 1,
        READ        = 1 << 2,
        WRITE       = 1 << 3,
        APPEND      = 1 << 4,
        TRUNCATE    = 1 << 5,
        PATCH       = 1 << 6,
        MARK        = 1 << 7,
    };

    uint32 properties;
};
    
}

}}}
