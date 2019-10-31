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

typedef out_netbuf_streambuf<char, embr::lwip::PbufNetbuf> out_pbuf_streambuf;
typedef in_netbuf_streambuf<char, embr::lwip::PbufNetbuf> in_pbuf_streambuf;
typedef estd::internal::basic_ostream<out_pbuf_streambuf> pbuf_ostream;
typedef estd::internal::basic_istream<in_pbuf_streambuf> pbuf_istream;

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

TEST_CASE("lwip pbuf embr-netbuf: ostream", "[lwip-pbuf]")
{
    // NOTE: Compiles, not runtime tested at all
    estd::internal::basic_ostream<out_netbuf_streambuf<char, embr::lwip::PbufNetbuf>> 
        out(netbuf_size);

    out << s1;

    auto sb = out.rdbuf();

    estd::layer3::const_string s(sb->pbase(), s1_size);

    ESP_LOGI(TAG, "sz = %d", sb->pptr() - sb->pbase());

    TEST_ASSERT(s == s1);
}


TEST_CASE("lwip pbuf embr-netbuf: in streambuf", "[lwip-pbuf]")
{
    in_pbuf_streambuf sb(netbuf_size);
}