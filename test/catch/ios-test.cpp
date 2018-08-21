#include <catch.hpp>

#include <embr/streambuf.h>
#include <embr/netbuf-static.h>
#include <embr/netbuf-dynamic.h>

#include <estd/string.h>
#include <estd/ostream.h>

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
        //c1->next(c2);
        nb2.chunks.insert_after(nb2.chunks.begin(), *c2);

        for(auto i : nb2.chunks)
        {

        }
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

        mem::experimental::NetBufDynamic<>* _nb2 = new mem::experimental::NetBufDynamic<>();
        mem::experimental::NetBufDynamic<>& nb2 = *_nb2;

        // FIX: this breaks, something about intrusive list
        nb2.expand(nb2sz, false);

        // any iteration thru the nb2 linked list dies

        SECTION("streambuf")
        {
            mem::netbuf_streambuf<char, decltype (nb2)&> sb(nb2);

            sb.sputc('a');

            REQUIRE(nb2.size() == nb2sz);
            REQUIRE(nb2.total_size() == nb2sz);

            sb.sputn(test_str.data(), test_str.size());

            REQUIRE(nb2.total_size() == test_str.size() + 1);

            SECTION("ostream")
            {
                estd::internal::basic_ostream<decltype (sb)&> out(sb);

                // FIX: Encounters a problem, can't resolve
                //out << test_str;
                out << test_str.data();
            }

        }

        // Just making sure it's not some wacky scoping thing that catch.hpp does
        // and it isn't.
        delete _nb2;
    }
}
