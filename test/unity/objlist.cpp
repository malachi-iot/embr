#include "unit-test.h"

#include <embr/storage/objlist.h>
#include <embr/storage/funclist.h>

static void test_funclist()
{
    int counter = 0;
    using objlist_type = embr::layer1::objlist<64>;
    objlist_type objlist;
    embr::detail::funclist<void(int), objlist_type> list(&objlist); //{&objlist};

    list += [&](int v) { counter += v; };
    list.fire(5);

    TEST_ASSERT_EQUAL(5, counter);
}

#ifdef ESP_IDF_TESTING
TEST_CASE("objlist", "[objlist]")
#else
void test_objlist()
#endif
{
    RUN_TEST(test_funclist);
}