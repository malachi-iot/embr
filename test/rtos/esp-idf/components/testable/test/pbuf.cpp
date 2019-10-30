#include <embr/platform/lwip/pbuf.h>
#include <embr/streambuf.h>
#include <estd/string.h>

#include "unity.h"

using namespace embr;
using namespace embr::mem;

TEST_CASE("lwip pbuf embr-netbuf", "[lwip-pbuf]")
{
    // Needing to do reference version since:
    // a) it's the more likely short term use case
    // b) move constructors aren't in place yet, causing havoc with pbuf alloc/free
    // FIX: Still glitching, presumably because presumably pbuf
    // isn't available until WiFi itself finishes initializing
    
    embr::lwip::PbufNetbuf _netbuf(128);
    out_netbuf_streambuf<char, embr::lwip::PbufNetbuf&> sb(_netbuf);

    sb.xsputn("test", 4);

    TEST_ASSERT(true);
}