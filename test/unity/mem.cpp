#include "unit-test.h"

//#include <embr/mem/v1/pool.hpp>

#ifdef ESP_IDF_TESTING
TEST_CASE("gc memory allocator", "[gc]")
#else
void test_mem_gc()
#endif
{

}