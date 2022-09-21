#include "esp_log.h"

#include <embr/platform/lwip/iostream.h>

#if FEATURE_EMBR_NETBUF_STREAMBUF
using embr::lwip::legacy::opbufstream;
using embr::lwip::legacy::ipbufstream;
#else
using embr::lwip::opbufstream;
using embr::lwip::ipbufstream;
#endif

void process_out(ipbufstream& in, opbufstream& out);

