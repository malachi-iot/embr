#include <embr/dataport.hpp>
#include <embr/datapump.hpp>
#include <embr/observer.h>
#include <embr/platform/lwip/dataport.h>

#include "unity.h"

#include "esp_log.h"


struct Empty {};

TEST_CASE("dataport test 1", "[dataport]")
{
    auto subject = embr::layer0::subject<Empty>();
    auto dataport = embr::lwip::make_udp_dataport(subject, 7);
}