#include <catch2/catch_all.hpp>

#include <embr/storage/objlist.h>
#include <embr/storage/funclist.h>

template <class Objstack>
unsigned total_allocated(embr::detail::v1::objlist<Objstack>& objlist)
{
    using objlist_type = embr::detail::v1::objlist<Objstack>;
    using pointer = typename objlist_type::const_pointer;

    unsigned sz = 0;

    objlist.walk([&](pointer p)
    {
        if(p->allocated())
            sz += p->size();
    });

    return sz;
}



TEST_CASE("Object list, Object stack", "[objlist]")
{
    using objlist_type = embr::layer1::objlist<256>;
    using element_type = objlist_type::value_type;
    using pointer = objlist_type::pointer;
    using objlist3_type = embr::layer3::objlist;
    char temp[256];

    SECTION("objstack")
    {
        SECTION("layer2")
        {
            embr::layer2::v1::objstack<256> stack(temp);
        }
        SECTION("layer3")
        {
            embr::layer3::v1::objstack stack(temp);
        }
    }
    SECTION("particulars")
    {
        element_type elem(0, -1);
        element_type elem2(0, 0);

        REQUIRE(elem.next_diff() == -4);
        REQUIRE(elem2.next() == nullptr);

        elem.next(&elem2);

        pointer next = elem.next();

        REQUIRE(next == &elem2);
    }
    SECTION("layer1: list")
    {
        objlist_type objlist;

        pointer e1a = objlist.alloc(nullptr, 16);
        pointer e2a = objlist.alloc(e1a, 16);

        REQUIRE(e1a->next() == e2a);

        pointer e1b = objlist.alloc(nullptr, 33);
        pointer e2b = objlist.alloc(e1b, 32);

        // FIX: Looks like a precision loss/miscalculation on our alignment trick
        pointer next = e1b->next();

        REQUIRE(next == e2b);

        REQUIRE(total_allocated(objlist) == 32 + 65);

        objlist.dealloc_next(e1b);

        REQUIRE(total_allocated(objlist) == 32 + 33);

        objlist.alloc(e2b, 5);
    }
    SECTION("layer3: list")
    {
        objlist3_type list(temp);

        pointer e1a = list.alloc(nullptr, 16);
        pointer e2a = list.alloc(e1a, 16);

        REQUIRE((char*)e1a == temp);
        REQUIRE(((char*)e2a) + e2a->total_size()  == list.stack().current());

        list.dealloc_next(e1a);

        REQUIRE((char*)e2a == list.stack().current());
    }
    SECTION("funclist")
    {
        int counter = 0;
        objlist_type objlist;
        embr::detail::funclist<void(int), objlist_type> list(&objlist); //{&objlist};
        list += [&](int v) { counter += v; };
        list.fire(5);

        REQUIRE(counter == 5);

        embr::detail::funclist<void(int), objlist_type> list2(&objlist);

        list2 += [&](int v) { counter -= v; };
        list2.fire(1);

        REQUIRE(counter == 4);
    }
}
