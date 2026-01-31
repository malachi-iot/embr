#include "unit-test.h"

#include <embr/platform/freertos/mem/pool.hpp>

#ifdef ESP_IDF_TESTING
TEST_CASE("gc memory allocator", "[gc]")
#else
void test_mem_gc()
#endif
{

}