// OTS_v10.pdf [1.2] Section 3.4

#include "../fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt {

inline namespace v1 {

PACK(struct ObjectSize
{
    uint32 current;
    uint32 allocated;
});
    
}

}}}
