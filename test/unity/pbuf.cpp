#include <embr/platform/lwip/iostream.h>
#include <embr/streambuf.h>

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

//#include <estd/iostream.h>    // FIX: This fails rather badly, look into why

#include "unit-test.h"

#include "esp_log.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

using namespace embr;
using namespace embr::mem;

const char* s1 = "test";
const char* s2 = "longer test string";
constexpr int s1_size = 4;
constexpr int s2_size = 17;
constexpr int netbuf_size = 128;

static const char* TAG = "lwip-pbuf";

using embr::lwip::opbufstream;
using embr::lwip::ipbufstream;

typedef embr::lwip::opbuf_streambuf out_pbuf_streambuf;
typedef embr::lwip::ipbuf_streambuf in_pbuf_streambuf;

typedef in_pbuf_streambuf::traits_type traits_type;

TEST_CASE("lwip pbuf: in streambuf", "[lwip-pbuf]")
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



TEST_CASE("lwip pbuf: in seekoff", "[lwip-pbuf]")
{
    constexpr int netbuf_size = 64;

    char buf[netbuf_size];
    in_pbuf_streambuf sb(netbuf_size);

    sb.pubseekoff(1, estd::ios_base::cur);

    TEST_ASSERT_EQUAL_INT(1, sb.pos());

    int read_back = sb.sgetn(buf, netbuf_size / 2);

    TEST_ASSERT_EQUAL_INT(1 + netbuf_size / 2, sb.pos());

}

TEST_CASE("lwip pbuf out stream: shrink", "[lwip-pbuf]")
{
    char buf[netbuf_size];
    out_pbuf_streambuf sb(netbuf_size);

    int read_back = sb.sputn(buf, netbuf_size / 2);

    TEST_ASSERT_EQUAL_INT(netbuf_size / 2, read_back);

    sb.shrink();

    embr::lwip::Pbuf& pbuf = sb.pbuf();

    TEST_ASSERT_EQUAL_INT(read_back, pbuf.total_length());
}

TEST_CASE("lwip pbuf stream: out seekoff", "[lwip-pbuf]")
{
    char buf[netbuf_size];
    out_pbuf_streambuf sb(netbuf_size);

    sb.pubseekoff(1, estd::ios_base::cur);

    TEST_ASSERT_EQUAL_INT(1, sb.pos());

    int read_back = sb.sputn(buf, netbuf_size / 2);

    TEST_ASSERT_EQUAL_INT(1 + netbuf_size / 2, sb.pos());
    TEST_ASSERT_EQUAL_INT(netbuf_size / 2, read_back);
}

TEST_CASE("lwip pbuf stream: istream", "[lwip-pbuf]")
{
    using namespace estd;

    ipbufstream in(netbuf_size);
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
        ipbufstream temp(p);
        TEST_ASSERT(p->ref == 2);
    }
    TEST_ASSERT(p->ref == 1);
    {
        // these parameters mean spin up istream using existing pbuf
        // and DON'T increase ref counter
        ipbufstream temp(p, false);
        TEST_ASSERT(p->ref == 1);
    }
    // WARNING: p may in fact be deallocated at this point, so if
    // we get failures here, comment out this line
    TEST_ASSERT(p->ref == 0);
}



TEST_CASE("lwip streambuf: helpers", "[lwip-helpers]")
{
    CONSTEXPR unsigned pbuf_size = 32;
    embr::lwip::Pbuf pbuf(pbuf_size);
    embr::lwip::PbufBase pbuf2(pbuf_size);

    pbuf.concat(pbuf2);

    embr::lwip::Pbuf::size_type size = embr::lwip::delta_length(pbuf, pbuf2);

    TEST_ASSERT_EQUAL(pbuf_size, size);

    pbuf.concat(pbuf2 = embr::lwip::PbufBase(pbuf_size));

    size = embr::lwip::delta_length(pbuf, pbuf2);

    TEST_ASSERT_EQUAL(pbuf_size * 2, size);
    TEST_ASSERT_EQUAL(pbuf_size * 3, pbuf.total_length());

    embr::lwip::PbufBase pbuf3 = pbuf.skip(pbuf_size * 2, &size);

    TEST_ASSERT(pbuf2 == pbuf3);
    TEST_ASSERT_EQUAL(size, 0);
}

static void streambuf_output_1()
{
    CONSTEXPR unsigned pbuf_size = 32;
    embr::lwip::Pbuf pbuf(pbuf_size);
    embr::lwip::basic_opbuf_streambuf<char> out(std::move(pbuf));

    out.sputn(s1, s1_size);

    TEST_ASSERT_EQUAL(s1_size, out.pos());
    char* payload = out.pbase();
    TEST_ASSERT(payload != nullptr);
#ifndef ESP_IDF_TESTING
    TEST_ASSERT_EQUAL_CHAR_ARRAY(s1, payload, s1_size);
#else
    TEST_ASSERT_EQUAL(s1[0], *payload);
#endif

    int r = out.sputc('A');

    TEST_ASSERT_EQUAL(s1_size + 1, out.pos());
    TEST_ASSERT(r != -1);
    TEST_ASSERT_EQUAL('A', payload[s1_size]);
    
    out.shrink();

    TEST_ASSERT_EQUAL(s1_size + 1, out.pbuf().total_length());
}

// There may be a bug where output streambuf aggressively expands its size
// This unit test is to diagnose that
// DEBT: Once diagnostic is complete, change above comment
static void streambuf_output_2()
{
    CONSTEXPR unsigned pbuf_size = 128;
    embr::lwip::Pbuf pbuf(pbuf_size);
    embr::lwip::opbuf_streambuf out(pbuf);

    out.sputn(s1, s1_size);
    out.sputn(s2, 3);
    out.sputn(s2, 2);

    TEST_ASSERT_EQUAL(pbuf_size, pbuf.total_length());

    out.sputn(s2, s2_size);
    out.sputn(s2, s2_size);

    TEST_ASSERT_EQUAL(pbuf_size, pbuf.total_length());
    TEST_ASSERT_EQUAL(pbuf_size, pbuf.length());
    TEST_ASSERT_EQUAL(pbuf_size, delta_length(pbuf, pbuf.pbuf()->next));
}

// Testing overflow and experimental 'grow_by'
static void streambuf_output_3()
{
    estd::internal::streambuf<
        embr::lwip::impl::opbuf_streambuf<estd::char_traits<char>, 32> > out(32);

    out.sputn(s2, s2_size);
    out.sputn(s2, s2_size);

    TEST_ASSERT_EQUAL(64, out.pbuf().total_length());
}

// Testing boundary when streambuf enters 'grow_by' mode
static void streambuf_output_grow()
{
    embr::lwip::opbuf_streambuf out(8);

    out.sputn("0123456789", 10);
    
    out.shrink();

    embr::lwip::PbufBase pbuf(out.pbuf());

    auto payload = static_cast<const char*>(pbuf.payload());

    TEST_ASSERT_EQUAL('1', payload[1]);
    TEST_ASSERT_EQUAL('7', payload[7]);

    // Remember, this is chained, so payload[8] is not present.

    pbuf = pbuf.next();

    TEST_ASSERT_EQUAL('8', pbuf.get_at(0));
    TEST_ASSERT_EQUAL('9', pbuf.get_at(1));

    TEST_ASSERT_EQUAL(10, out.pbuf().total_length());
}

TEST_CASE("lwip streambuf: output", "[lwip-streambuf]")
{
    streambuf_output_1();
    streambuf_output_2();
    streambuf_output_3();
    streambuf_output_grow();
}


TEST_CASE("lwip streambuf: input", "[lwip-streambuf]")
{
    // remember, pbufs are assumed to have the entire content populated.  This
    // test we only actually populate s1_size amount
    embr::lwip::Pbuf pbuf(128);
    
    char* payload = (char*)pbuf.payload();
    strcpy(payload, s1);

    embr::lwip::ipbuf_streambuf in(std::move(pbuf));

    TEST_ASSERT_EQUAL(0, in.pubseekoff(0, estd::ios_base::cur));
    TEST_ASSERT_EQUAL(payload, in.eback());
    // DEBT: If showmanyc doesn't get called properly, a full recompile
    // may be necessary.  Debt because that is a cmake/.h file detection
    // problem
    TEST_ASSERT_EQUAL(128, in.in_avail());

    char buf[128];

    int size = in.sgetn(buf, s1_size + 1);

    TEST_ASSERT_EQUAL(s1_size + 1, size);
    TEST_ASSERT_EQUAL(s1[0], buf[0]);
}

TEST_CASE("lwip istream", "[lwip-ios]")
{
    embr::lwip::Pbuf pbuf(128);

    char* payload = (char*)pbuf.payload();
    char buf[128];

    embr::lwip::ipbufstream in(std::move(pbuf));

    in.ignore(128);

    int val = in.get();

    TEST_ASSERT_EQUAL(-1, val);
}

TEST_CASE("lwip ostream", "[lwip-ios]")
{
    embr::lwip::Pbuf pbuf(128);

    char* payload = (char*)pbuf.payload();

    embr::lwip::opbufstream out(std::move(pbuf));

    out << s1 << estd::endl;

    TEST_ASSERT_EQUAL(s1_size + 1, out.tellp());
    TEST_ASSERT_EQUAL(s1[0], *payload);
}

#pragma GCC diagnostic pop

