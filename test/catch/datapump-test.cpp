#include <catch.hpp>

#include <embr/datapump.hpp>
#include <embr/netbuf-static.h>

typedef embr::mem::layer1::NetBuf<128> synthetic_netbuf_type;
typedef embr::TransportDescriptor<synthetic_netbuf_type, int> synthetic_transport_descriptor;

TEST_CASE("datapump")
{
    synthetic_netbuf_type nb;
    embr::DataPump<synthetic_transport_descriptor> synthetic_datapump;

    SECTION("A")
    {

    }
}
