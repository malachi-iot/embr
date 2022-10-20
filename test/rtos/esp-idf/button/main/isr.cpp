#include <driver/gpio.h>

#include <embr/platform/esp-idf/debounce.hpp>

#include "main.h"
#include "gpio.h"
#include "xthal_clock.h"

using namespace embr::detail;
using namespace estd::chrono_literals;

static constexpr gpio_num_t pin = (gpio_num_t)CONFIG_BUTTON_PIN;

void test_isr()
{

}