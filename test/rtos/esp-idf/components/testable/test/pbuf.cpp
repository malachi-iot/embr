#include <embr/platform/lwip/pbuf.h>
#include <embr/streambuf.h>

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

//#include <estd/iostream.h>    // FIX: This fails rather badly, look into why

#include "unity.h"

#include "esp_log.h"

using namespace embr;
using namespace embr::mem;

const char* s1 = "test";
constexpr int s1_size = 4;
constexpr int netbuf_size = 128;

static const char* TAG = "lwip-pbuf";

typedef embr::lwip::PbufNetbuf netbuf_type;
typedef netbuf_type::size_type size_type;
typedef out_netbuf_streambuf<char, netbuf_type> out_pbuf_streambuf;
typedef in_netbuf_streambuf<char, netbuf_type> in_pbuf_streambuf;
typedef estd::internal::basic_ostream<out_pbuf_streambuf> pbuf_ostream;
typedef estd::internal::basic_istream<in_pbuf_streambuf> pbuf_istream;
typedef in_pbuf_streambuf::traits_type traits_type;

TEST_CASE("lwip pbuf embr-netbuf: out streambuf", "[lwip-pbuf]")
{
    // NOTE: Not tested with move constructor, only tested with reference version
    embr::lwip::PbufNetbuf _netbuf(netbuf_size);

    TEST_ASSERT(_netbuf.size() == netbuf_size);

    //out_netbuf_streambuf<char, embr::lwip::PbufNetbuf&> sb(_netbuf);
    out_netbuf_streambuf<char, embr::lwip::PbufNetbuf> sb(std::move(_netbuf));
    //out_netbuf_streambuf<char, embr::lwip::PbufNetbuf> sb(netbuf_size);

    sb.xsputn(s1, s1_size);

    auto sz = sb.pptr() - sb.pbase();

    TEST_ASSERT(sz == s1_size);

    // FIX: None of these work
    //estd::layer2::string<s1_size, false> _s((char*)_netbuf.data());
    estd::layer2::string<s1_size, false> _s(sb.pbase());
    //estd::layer2::string<s1_size> _s(s1);
    //estd::layer2::basic_string<const char, s1_size> _s((char*)_netbuf.data());

    ESP_LOGI(TAG, "sz = %d, _s.size() = %d", sz, _s.size());

    // FIX: _s.size() comes out to 0 somehow
    //TEST_ASSERT(_s.size() == s1_size);

    estd::layer3::const_string s(sb.pbase(), s1_size);
    //estd::layer2::const_string s(s1);

    TEST_ASSERT(s == s1);
}

#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
TEST_CASE("lwip pbuf embr-netbuf: out streambuf chain", "[lwip-pbuf]")
{
    constexpr int netbuf_size = 64;

    out_pbuf_streambuf sb(netbuf_size);
    
    const netbuf_type& netbuf = sb.cnetbuf();

    ESP_LOGI(TAG, "#1 total_size = %d", netbuf.total_size());
    TEST_ASSERT(netbuf.total_size() == netbuf_size);

    for(int i = 0; i < 1 + (netbuf_size * 3); i += s1_size)
    {
        sb.sputn(s1, s1_size);
    }

    ESP_LOGI(TAG, "#2 total_size = %d", netbuf.total_size());
    TEST_ASSERT(netbuf.total_size() == ((netbuf_size * 3) + netbuf_type::threshold_size));
}
#endif

TEST_CASE("lwip pbuf embr-netbuf: ostream", "[lwip-pbuf]")
{
    // NOTE: Compiles, not runtime tested at all
    typedef estd::internal::streambuf<
        out_netbuf_streambuf<char, embr::lwip::PbufNetbuf> > streambuf_type;

    estd::internal::basic_ostream<streambuf_type> 
        out(netbuf_size);

    out << s1;

    auto sb = out.rdbuf();

    estd::layer3::const_string s(sb->pbase(), s1_size);

    ESP_LOGI(TAG, "sz = %d", sb->pptr() - sb->pbase());

    TEST_ASSERT(s == s1);
}


TEST_CASE("lwip pbuf embr-netbuf: in streambuf", "[lwip-pbuf]")
{
    // NOTE: Initializing an empty input pbuf with a large size like this is
    // a little unusual.  Generally the system provides us with an incoming
    // pbuf already populated.  We'd only populate the pbuf if we ourselves
    // were handling the transport
    in_pbuf_streambuf sb(netbuf_size);
    char buf[netbuf_size];

    // reads a bunch of uninitialized characters back
    int read_back = sb.sgetn(buf, netbuf_size / 2);

    TEST_ASSERT(read_back == netbuf_size / 2);
}


#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
// discere input vs output streambufs
TEST_CASE("lwip pbuf embr-netbuf: out+in streambuf chain 1", "[lwip-pbuf]")
{
    constexpr int netbuf_size = 64;

    out_pbuf_streambuf sb_out(netbuf_size);
    
    const netbuf_type& out_netbuf = sb_out.cnetbuf();

    for(int i = 0; i < 1 + (netbuf_size * 3); i += s1_size)
    {
        sb_out.sputn(s1, s1_size);
    }

    in_pbuf_streambuf sb(out_netbuf, true);

    const netbuf_type& in_netbuf = sb.cnetbuf();

    TEST_ASSERT_EQUAL_INT(0, sb.pos());
    TEST_ASSERT_EQUAL_INT(netbuf_size, in_netbuf.size());

    char buf[netbuf_size];

    // NOTE: Still figuring out if this is total size or chunk size
    // at the moment, it's total size
    TEST_ASSERT_EQUAL_INT(224, sb.in_avail());

    int read_back = sb.sbumpc();

    TEST_ASSERT(sb.pos() == 1);
    
    TEST_ASSERT(read_back == s1[0]);

    read_back = sb.sgetn(buf, netbuf_size / 2);

    TEST_ASSERT_EQUAL_INT(1 + netbuf_size / 2, sb.pos());

    TEST_ASSERT(read_back == netbuf_size / 2);
}
#endif

TEST_CASE("lwip pbuf embr-netbuf: in seekoff", "[lwip-pbuf]")
{
    constexpr int netbuf_size = 64;

    char buf[netbuf_size];
    in_pbuf_streambuf sb(netbuf_size);

    sb.pubseekoff(1, estd::ios_base::cur);

    TEST_ASSERT_EQUAL_INT(1, sb.pos());

    int read_back = sb.sgetn(buf, netbuf_size / 2);

    TEST_ASSERT_EQUAL_INT(1 + netbuf_size / 2, sb.pos());

}

TEST_CASE("lwip pbuf embr-netbuf: out seekoff", "[lwip-pbuf]")
{
    char buf[netbuf_size];
    out_pbuf_streambuf sb(netbuf_size);

    sb.pubseekoff(1, estd::ios_base::cur);

    TEST_ASSERT_EQUAL_INT(1, sb.pos());

    int read_back = sb.sputn(buf, netbuf_size / 2);

    TEST_ASSERT_EQUAL_INT(1 + netbuf_size / 2, sb.pos());
}

TEST_CASE("lwip pbuf embr-netbuf: istream", "[lwip-pbuf]")
{
    using namespace estd;

    pbuf_istream in(netbuf_size);
    char buf[netbuf_size];

    int read_back = in.readsome(buf, netbuf_size / 2);

    TEST_ASSERT(read_back == netbuf_size / 2);

    //in.read(buf, netbuf_size / 2);

    /* NOTE: estd doesn't have these input >> operators built out yet
    int throwaway;

    in >> throwaway; */

    auto* p = pbuf_alloc(PBUF_TRANSPORT, 1, PBUF_RAM);

    TEST_ASSERT(p->ref == 1);
    {
        // these parameters mean spin up istream using existing pbuf
        // and increases ref counter
        pbuf_istream temp(p);
        TEST_ASSERT(p->ref == 2);
    }
    TEST_ASSERT(p->ref == 1);
    {
        // these parameters mean spin up istream using existing pbuf
        // and DON'T increase ref counter
        pbuf_istream temp(p, false);
        TEST_ASSERT(p->ref == 1);
    }
    // WARNING: p may in fact be deallocated at this point, so if
    // we get failures here, comment out this line
    TEST_ASSERT(p->ref == 0);
}


#ifdef FEATURE_EMBR_PBUF_CHAIN_EXP
TEST_CASE("lwip pbuf embr-netbuf: netbuf shrink", "[lwip-pbuf]")
{
    // UNFINISHED
    size_type constexpr threshold_size = netbuf_type::threshold_size;
    int rotation_size = 50;
    int rotations = 6; // presuming these are gonna be bigger than threshold size, for now

    pbuf_ostream out(threshold_size);

    // assumes ASCII
    const char ch = '0';

    for(int count = 0; count < rotations; count++)
        for(int r = 0; r < rotation_size; r++)
            out.put(ch + r);

    netbuf_type& netbuf = out.rdbuf()->netbuf();

    size_type total_size = netbuf.total_size();
    size_type actual_size = out.rdbuf()->absolute_pos();

    TEST_ASSERT_EQUAL((rotation_size * rotations), actual_size);
    TEST_ASSERT_LESS_OR_EQUAL(total_size, actual_size);

    netbuf.shrink(actual_size);
    netbuf.reset();

    pbuf_istream in(netbuf.pbuf());

    for(int count = 0; count < rotations; count++)
        for(int r = 0; r < rotation_size; r++)
        {
            char ch_expected = ch + r;
            char ch_actual = in.get();

            TEST_ASSERT_EQUAL(ch_expected, ch_actual);
        }

    TEST_ASSERT_EQUAL(traits_type::eof(), in.get());
}
#endif
