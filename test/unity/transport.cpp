#include "unit-test.h"

// DEBT: Re-enable this on Arduino when lwip is present
#if !defined(ARDUINO) && defined(ESP_PLATFORM) || defined(EMBR_PICOW_BOARD)

#include <embr/platform/lwip/transport.hpp>
#include <embr/streambuf.h>

#include <estd/string.h>
#include <estd/ostream.h>
#include <estd/istream.h>

#include <embr/observer.h>

//#include <estd/iostream.h>    // FIX: This fails rather badly, look into why

#include "esp_log.h"

using namespace embr::lwip;
using namespace embr::lwip::experimental;

int constexpr pbuf_size = 128;

volatile bool always_false_but_compiler_doesnt_know = false;

// DEBT: Use loopback addresses so that we can actually queue a send

TEST_CASE("lwip transport: basic", "[lwip-transport]")
{
    typedef TransportUdp<> transport_type;
    
    transport_type::endpoint_type endpoint(nullptr, 0);

    transport_type::buffer_type buffer(128);
    
    transport_type t(nullptr);

    if(always_false_but_compiler_doesnt_know)
        t.send(buffer, endpoint);
}


TEST_CASE("lwip transport: streambuf", "[lwip-transport]")
{
    typedef TransportUdp<> transport_type;
    
    transport_type::endpoint_type endpoint(nullptr, 0);

    transport_type::ostreambuf_type streambuf(128);
    
    transport_type t(nullptr);

    if(always_false_but_compiler_doesnt_know)
        t.send(streambuf, endpoint);
}

TEST_CASE("lwip subject transport: basic", "[lwip-transport]")
{
    UdpSubjectTransport transport;

    // 
    embr::void_subject s;
    transport.recv(s, 0);
}

#endif