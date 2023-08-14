#include <embr/observer.h>

#include "unit-test.h"

static void test_layer0_subject()
{
    embr::layer0::subject<int> subject;

    TEST_ASSERT_EQUAL(1, sizeof(subject));
}


static void test_layer1_subject()
{
    embr::layer1::subject<int> subject;

    TEST_ASSERT_EQUAL(sizeof(int), sizeof(subject));
}

#ifdef ESP_IDF_TESTING
TEST_CASE("subject/observer tests", "[observer]")
#else
void test_observer()
#endif
{
    RUN_TEST(test_layer0_subject);
    RUN_TEST(test_layer1_subject);
}