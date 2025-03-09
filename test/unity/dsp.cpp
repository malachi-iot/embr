#include <embr/profiler.h>
#include <embr/dsp/precalc.h>

#include "unit-test.h"

using namespace embr;

static void test_sinf()
{

}

#ifdef ESP_IDF_TESTING
TEST_CASE("dsp tests", "[dsp]")
#else
void test_dsp()
#endif
{
#if FEATURE_EMBR_DSP_PRECALC_TABLE
    dsp::init_sin_table();
#endif

    RUN_TEST(test_sinf);
}
