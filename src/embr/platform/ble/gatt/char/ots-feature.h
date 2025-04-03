
// OTS_v10.pdf [1.2] Section 3.1

#include "fwd.h"
#include "../int.h"

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

PACK(struct OtsFeature
{
    uint32 oacp;
    uint32 olcp;
});

}}}}
