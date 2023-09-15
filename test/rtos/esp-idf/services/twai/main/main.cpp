#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

#include <embr/platform/esp-idf/board.h>
#include <embr/platform/esp-idf/gpio.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/twai.hpp>


using Diagnostic = embr::esp_idf::service::v1::Diagnostic;

#include "app.h"


void App::on_notify(TWAI::event::alert alert)
{
    ESP_LOGI(TAG, "on_notify: TWAI alert=%" PRIx32, alert.alerts);
}

void App::on_notify(TWAI::event::autorx rx)
{
    const twai_message_t& message = rx.message;
    
    ESP_LOGI(TAG, "on_notify: TWAI rx: id=%" PRIx32 " dlc=%u",
        message.identifier, message.data_length_code);
    ESP_LOG_BUFFER_HEX(TAG, message.data, message.data_length_code);
}


namespace app_domain {

App app;

typedef estd::integral_constant<App*, &app> app_singleton;
typedef embr::layer0::subject<Diagnostic, app_singleton> filter_observer;

embr::esp_idf::service::v1::TWAI::runtime<filter_observer> twai;

}

#if defined(CONFIG_BOARD_ESP32S3_UM_FEATHERS3)
#if CONFIG_GPIO_TWAI_RX == 4
#error "FeatherS3 can't operate TWAI RX on default pin 4"
#endif
#endif

#if !CONFIG_TWAI_TIMING
#error Please configure TWAI in Embr Helper area
#endif


using board_traits = embr::esp_idf::board_traits;


#if FEATURE_EMBR_BOARD_STATUS_LED
constexpr embr::esp_idf::gpio status_led(
    (gpio_num_t)board_traits::gpio::status_led);

void init_gpio()
{
    status_led.set_direction(GPIO_MODE_OUTPUT);
}
#else
embr::esp_idf::gpio status_led((gpio_num_t)-1);

// DEBT: While we work out our variadic selectors, temporarily depend on this guy
bool has_status_led() { return (int)status_led != -1; }

template <class Traits>
static void init_gpio1()
{
    static const char* TAG = "init_gpio1";

    namespace R = embr::esp_idf::R;

    // DEBT: Need a 0-element 'selector' as well, if we're gonna keep the paradigm
    // DEBT: Change 'selector' to 'select' in this context.  The underlying code
    // is more or less selector<>::selected which makes sense at that low level, but high
    // level not so much

    using selected = typename Traits::io::selector<R::traits_selector<R::led, R::trait::status> >;

    ESP_LOGD(TAG, "selected.size = %u", selected::size());

    // This next part requires c++17
    if constexpr (selected::size() > 0)
    {
        using mux = selected::first::type;

        ESP_LOGI(TAG, "status LED pin=%u", mux::pin);

        status_led = embr::esp_idf::gpio(mux::pin);
        status_led.set_direction(GPIO_MODE_OUTPUT);

        if constexpr (mux::traits::template selector<R::is_in_selector<R::color::blue> >::size() > 0)
        {
            ESP_LOGD(TAG, "status LED color=blue");
        }
    }
    else
    {

    }
}

void init_gpio()
{
    init_gpio1<board_traits>();
}
#endif

#if defined(CONFIG_BOARD_ESP32_UNSPECIFIED)
#warning "Board not specified, but probably you'd prefer it to be"
#endif



extern "C" void app_main()
{
    const char* TAG = "app_main";

    ESP_LOGI(TAG, "Board: %s %s", board_traits::vendor, board_traits::name);

    app_domain::twai.start();
    app_domain::twai.autorx(true);
#ifdef CONFIG_TWAI_TEST_TASK
    app_domain::twai.start_task();
#endif

    init_gpio();

    for(;;)
    {
        static int counter = 0;

#ifdef CONFIG_TWAI_TEST_TASK
        estd::this_thread::sleep_for(estd::chrono::seconds(2));
#else
        app_domain::twai.poll(pdMS_TO_TICKS(5000));
#endif

        ESP_LOGI(TAG, "counting: %d", ++counter);

#if FEATURE_EMBR_BOARD_STATUS_LED
        status_led.level(counter % 2 == 0);
#else
        if(has_status_led())    status_led.level(counter % 2 == 0);
#endif
    }
}

