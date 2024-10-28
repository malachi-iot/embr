#include <estd/chrono.h>
#include <estd/cstdint.h>

#include "../fwd.h"
#include "../int.h"

// See ETS_v1.0.pdf
// See GATT_Specification_Supplement.pdf (2024-02-21) 3.77 p.90

namespace embr { namespace ble { namespace gatt { inline namespace v1 {

PACK(struct DateTime    // NOLINT
{
    uint16 year;
    uint8 month;
    uint8 day;
    uint8 hours;
    uint8 minutes;
    uint8 seconds;
});

}}}}    // embr::ble::gatt::v1