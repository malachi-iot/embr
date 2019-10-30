#include <embr/platform/lwip/pbuf.h>
#include <embr/streambuf.h>
#include <estd/string.h>

#include "unity.h"

using namespace embr;
using namespace embr::mem;

const char* s1 = "test";
constexpr int s1_size = 4;
constexpr int netbuf_size = 128;

TEST_CASE("lwip pbuf embr-netbuf", "[lwip-pbuf]")
{
    // Needing to do reference version since:
    // a) it's the more likely short term use case
    // b) move constructors aren't in place yet, causing havoc with pbuf alloc/free
    
    embr::lwip::PbufNetbuf _netbuf(netbuf_size);

    TEST_ASSERT(_netbuf.size() == netbuf_size);

    out_netbuf_streambuf<char, embr::lwip::PbufNetbuf&> sb(_netbuf);

    sb.xsputn(s1, s1_size);

    auto sz = sb.pptr() - sb.pbase();

    TEST_ASSERT(sz == s1_size);

    // FIX: None of these work
    //estd::layer2::string<s1_size, false> s((char*)_netbuf.data());
    //estd::layer2::string<s1_size, false> s(sb.pbase());
    //estd::layer2::string<s1_size> s(s1);
    //estd::layer2::basic_string<const char, s1_size> s((char*)_netbuf.data());

    estd::layer3::const_string s(sb.pbase(), s1_size);
    //estd::layer2::const_string s(s1);

    TEST_ASSERT(s == s1);
}