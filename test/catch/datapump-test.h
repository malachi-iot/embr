#pragma once

#include <embr/datapump.h>
#include <embr/obsolete/netbuf-static.h>
#include <embr/transport-descriptor.h>


typedef embr::mem::layer1::NetBuf<128> synthetic_netbuf_type;
typedef embr::TransportDescriptor<synthetic_netbuf_type, int> synthetic_transport_descriptor;
typedef embr::DataPump<synthetic_transport_descriptor> synthetic_datapump;
