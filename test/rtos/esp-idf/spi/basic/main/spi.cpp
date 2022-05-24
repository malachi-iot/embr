#include <estd/chrono.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/spi.h>

#include <esp_log.h>
#include <driver/gpio.h>

using namespace embr::esp_idf;

static spi::bus bus(HSPI_HOST);

void spi_init()
{
}


void spi_loop()
{

}