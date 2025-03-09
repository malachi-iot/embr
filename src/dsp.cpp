#include "embr/dsp/precalc.h"

namespace embr { namespace dsp { inline namespace v1 {

#if FEATURE_EMBR_DSP_PRECALC_TABLE
namespace detail {
float sin_table[EMBR_DSP_PRECALC_TABLE_SZ];
//float cos_table[EMBR_DSP_PRECALC_TABLE_SZ];
}

void init_sin_table()
{
    detail::precalc<PRECALC_DEFAULT>::init_sin(estd::span<float, EMBR_DSP_PRECALC_TABLE_SZ>{detail::sin_table});
}
#endif

}}}
