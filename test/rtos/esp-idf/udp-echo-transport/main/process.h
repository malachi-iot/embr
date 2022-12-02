#include "esp_log.h"

#include <embr/platform/lwip/iostream.h>

using embr::lwip::opbufstream;
using embr::lwip::ipbufstream;

void process_out(ipbufstream& in, opbufstream& out);

