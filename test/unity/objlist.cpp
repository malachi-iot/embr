#include "unit-test.h"

#include <embr/storage/objlist.h>
#include <embr/storage/funclist.h>

static void test_funclist1()
{
    int counter = 0;
    using objlist_type = embr::layer1::objlist<2048>;
    static objlist_type objlist;
    embr::detail::funclist<void(int), objlist_type> list(&objlist);

    list += [&](int v) { counter += v; };
    list += [&](int v) { counter += 1; };

    list.fire(5);

    TEST_ASSERT_EQUAL(6, counter);
}

static void test_funclist2()
{
    int counter = 0;
    using objlist_type = embr::layer1::objlist<64>;
    objlist_type objlist;
    embr::detail::funclist<void(int), objlist_type> list(&objlist);

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
    RUN_TEST(test_funclist1);
    RUN_TEST(test_funclist2);
}