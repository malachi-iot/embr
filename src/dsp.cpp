#include "embr/dsp/precalc/sin.h"

namespace embr { namespace dsp { inline namespace v1 {

#if FEATURE_EMBR_DSP_PRECALC_TABLE
namespace detail {
#if FEATURE_EMBR_DSP_PRECALC_TABLE_STATIC
float sin_table[EMBR_DSP_PRECALC_TABLE_SZ];
#else
float* sin_table;
#endif
//float cos_table[EMBR_DSP_PRECALC_TABLE_SZ];
}

static void init_sin_table_(float* table)
{
    detail::precalc<PRECALC_DEFAULT>::init_sin(estd::span<float, EMBR_DSP_PRECALC_TABLE_SZ>{table});
}

#if FEATURE_EMBR_DSP_PRECALC_TABLE_STATIC
void init_sin_table() { init_sin_table_(detail::sin_table); }
#else
void init_sin_table(float* table) { init_sin_table_(table); }
#endif

#endif

}}}
