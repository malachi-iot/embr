#include "unit-test.h"

#include <embr/storage/objlist.h>
#include <embr/storage/funclist.h>

class TestObject
{

};

static void test_objlist1()
{
    using objlist_type = embr::layer1::objlist<128>;
    static objlist_type objlist;

    //objlist.alloc()
}

static void test_funclist1()
{
    int counter = 0;
    using objstack_type = embr::layer1::objstack<128, 3>;
    using objlist_type = embr::detail::objlist<objstack_type>;
    static objlist_type objlist;
    embr::detail::funclist<void(int), objlist_type> list(&objlist);

    list += [&](int v) { counter += v; };
    list += [&](int v) { counter += 1; };

    // FIX
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
    list += [&](int v) { counter += 1; };

    list.fire(5);

    TEST_ASSERT_EQUAL(6, counter);
}


#ifdef ESP_IDF_TESTING
TEST_CASE("objlist", "[objlist]")
#else
void test_objlist()
#endif
{
    RUN_TEST(test_objlist1);
    RUN_TEST(test_funclist1);
    RUN_TEST(test_funclist2);
}