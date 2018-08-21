#include <catch.hpp>

#include <embr/streambuf.h>
#include <embr/netbuf-static.h>
#include <embr/netbuf-dynamic.h>

#include <estd/string.h>
#include <estd/ostream.h>

#include <estd/string_view.h>

#include <numeric>

using namespace embr;

TEST_CASE("iostreams", "[ios]")
{
    // FIX: constructor botches for some reason on that one
    //estd::layer2::string<> test_str = "the quick brown fox jumped over the lazy dog";
    estd::layer2::const_string test_str = "the quick brown fox jumped over the lazy dog";
    mem::layer1::NetBuf<32> nb;
    constexpr int nb2sz = 32;

    SECTION("basic output netbuf+streambuf impl")
    {
        mem::impl::out_netbuf_streambuf<char, mem::layer1::NetBuf<32>& > sb(nb);

        sb.xsputn("hi2u", 5); // cheat and include null termination also

        // TODO: make typedef layer1::string include provision to specify null termination
        // FIX: estd::layer2::string<> doesn't compile
        //estd::layer2::string<> s((char*)nb.data());
        char* helper = reinterpret_cast<char*>(nb.data());

        estd::layer2::const_string s(helper);

        REQUIRE(s == "hi2u");
    }
    SECTION("proper netbuf_streambuf type (not impl) + ostream")
    {
        using namespace estd::internal;

        mem::netbuf_streambuf<char, mem::layer1::NetBuf<32>& > sb(nb);
        estd::internal::basic_ostream<decltype(sb)&> out(sb);

        out << 'a';

        REQUIRE(nb.data()[0] == 'a');

        // NOTE: Odd that this doesn't work.  no usings seem to clear it up either
        out << " nice day";

        REQUIRE(memcmp(nb.data(), "a nice day", 10)== 0);

        int sz = sizeof(sb);

        SECTION("Direct pointer access")
        {
            REQUIRE(sb.pbase() == (char*)nb.data());
        }
    }
    SECTION("list of NetBufDynamicChunk")
    {
        typedef mem::experimental::NetBufDynamicChunk Chunk;
        // FIX: Oops, typo in that name...
        estd::intrusive_forward_list<Chunk> chunks;
        auto a = std::allocator<uint8_t>();
        constexpr int chunksz = sizeof(Chunk) + nb2sz;


#ifdef TEST_MALLOC_VERSION
        Chunk* chunk1 = (Chunk*)malloc(chunksz);
        Chunk* chunk2 = (Chunk*)malloc(chunksz);
#else
        Chunk* chunk1 = (Chunk*)a.allocate(chunksz);
        Chunk* chunk2 = (Chunk*)a.allocate(chunksz);
#endif
        chunk1->size = nb2sz;
        chunk2->size = nb2sz;
        std::iota(chunk1->data, chunk1->data + nb2sz, 10);
        std::iota(chunk2->data, chunk2->data + nb2sz, 20);

        chunks.push_front(*chunk1);
        chunk1->next(chunk2);

        auto i = chunks.begin();

        Chunk* c_i = &i.lock();

        REQUIRE(c_i->data[0] == 10);

        ++i;

#ifdef TEST_MALLOC_VERSION
        free(c_i);
#else
        a.deallocate((uint8_t*)c_i, chunksz);
#endif
        c_i = &i.lock();

        REQUIRE(c_i->data[0] == 20);

        ++i;

#ifdef TEST_MALLOC_VERSION
        free(c_i);
#else
        a.deallocate((uint8_t*)c_i, chunksz);
#endif
    }
    SECTION("low-level-dynamic 1")
    {
        typedef mem::experimental::NetBufDynamicChunk Chunk;
        mem::experimental::NetBufDynamic<> nb2;

        Chunk* c1 = nb2.allocate(nb2sz);
        Chunk* c2 = nb2.allocate(nb2sz);

        nb2.chunks.push_front(*c1);
        c1->next(c2);
        //nb2.chunks.insert_after(nb2.chunks.begin(), *c2);
    }
    SECTION("low-level-dynamic 2")
    {
        mem::experimental::NetBufDynamic<> nb2;

        nb2.expand(nb2sz, true);
        std::iota(nb2.data(), nb2.data() + nb2sz, 0);
        nb2.expand(nb2sz, true);
        //std::iota(nb2.data(), nb2.data() + nb2sz, 0);

    }
    SECTION("dynamic")
    {
        INFO("examining");

        struct NoMinimumPolicy
        {
            CONSTEXPR int minimum_allocation_size() const { return 1; }
        };

        typedef mem::experimental::NetBufDynamic< std::allocator<uint8_t>, NoMinimumPolicy > netbuf_type;

        netbuf_type* _nb2 = new netbuf_type();
        netbuf_type& nb2 = *_nb2;

        nb2.expand(nb2sz, false);

        SECTION("streambuf")
        {
            estd::layer1::string<256> side_by_side;
            mem::netbuf_streambuf<char, decltype (nb2)&> sb(nb2);

            sb.sputc('a');
            side_by_side += 'a';

            REQUIRE(nb2.size() == nb2sz);
            REQUIRE(nb2.total_size() == nb2sz);

            sb.sputn(test_str.data(), test_str.size());
            side_by_side += test_str;

            REQUIRE(nb2.total_size() == test_str.size() + 1);

            SECTION("ostream")
            {
                estd::internal::basic_ostream<decltype (sb)&> out(sb);

                // FIX: Encounters a problem, can't resolve
                //out << test_str;
                out << test_str.data();
                side_by_side += test_str;
            }

            nb2.reset();

            // FIX: string_view should be easier to ascertain
            // not even this works
            /*
            estd::basic_string_view<char,
                    estd::layer1::string<256>::traits_type,
                    estd::layer1::string<256>::impl_type >
                    v = side_by_side; */
            {
                estd::string_view v(side_by_side.data(), side_by_side.size());

                // FIX: really could use substr and more advanced compares
            }

            const char* v = side_by_side.data();

            REQUIRE(nb2.size() == nb2sz);
            REQUIRE(memcmp(nb2.data(), v, nb2sz) == 0);
            v += nb2sz;
            REQUIRE(nb2.next());
            REQUIRE(nb2.size() == test_str.size() - nb2sz + 1);
            REQUIRE(memcmp(nb2.data(), v, nb2.size()) == 0);
            v += nb2.size();
            REQUIRE(nb2.next());
            REQUIRE(nb2.size() == test_str.size());
            REQUIRE(memcmp(nb2.data(), v, nb2.size()) == 0);
        }

        // Just making sure it's not some wacky scoping thing that catch.hpp does
        // and it isn't.
        delete _nb2;
    }
}
