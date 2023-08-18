#include <esp-helper.h>

#include <esp_event.h>
#include <esp_log.h>
#include "esp_err.h"
#include "esp_system.h"

#ifdef CONFIG_DIAGNOSTIC_PERFMON
#include <perfmon.h>
#endif

#include <estd/chrono.h>
#include <estd/thread.h>
#include <estd/internal/variadic.h>

#include <embr/platform/esp-idf/gpio.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/event.hpp>
#include <embr/platform/esp-idf/service/gptimer.hpp>

#include "app.h"
#include "event.h"

namespace service = embr::esp_idf::service::v1;
namespace debounce = embr::esp_idf::debounce::v1::ultimate;


#define GPIO_INPUT_IO_0     CONFIG_DIAGNOSTIC_GPIO1
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)

ESP_EVENT_DEFINE_BASE(DEBOUNCE_EVENT);


estd::tuple<
    debounce::Debouncer<GPIO_INPUT_IO_0, true>,
    debounce::Debouncer<4, true> > debouncers;    // DEBT: arbitrary selection of pin 4



void log(const char* TAG, const App::Event& item)
{
    ESP_LOGI(TAG, "pin: %u, event: %s (%u)",
        item.pin,
        embr::to_string(item.state),
        (unsigned)item.state);
}

inline void App::on_notify(Timer::event::callback)
{
#ifdef CONFIG_DIAGNOSTIC_PERFMON
    xtensa_perfmon_start();
#endif

    debouncers.visit(
        debounce::Visitor{},
        [this](Event e)
        {
            q.send_from_isr(e);
#ifdef CONFIG_ESP_EVENT_POST_FROM_ISR
            esp_event_isr_post(DEBOUNCE_EVENT, 0, &e, sizeof(e), nullptr);
#endif
        });

#ifdef CONFIG_DIAGNOSTIC_PERFMON
    xtensa_perfmon_stop();
#endif
}


inline void App::on_notify(Event e)
{
    log(TAG, e);
}


static void init_gpio_input()
{
    static const char* TAG = "init_gpio_input";

    ESP_LOGD(TAG, "entry");

    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    ESP_ERROR_CHECK(gpio_config(&io_conf));
}


#ifdef CONFIG_ESP_EVENT_POST_FROM_ISR
void debounce_event_handler(void*, esp_event_base_t, int32_t, void* event_data)
{
    static constexpr const char* TAG = "debounce_event_handler";

    log(TAG, * (App::Event*) event_data);
}
#endif


extern "C" void app_main()
{
    const char* TAG = "app_main";

    static App app;

    // wrap up our App in something ultra-compile time friendly
    typedef estd::integral_constant<App*, &app> app_singleton;
    using tier1 = embr::layer0::subject<service::Diagnostic, app_singleton>;

    // create timer_service with above specified observers
    service::GPTimer::runtime<tier1> timer;
    service::EventLoop::runtime<tier1> event_loop;

    event_loop.start();

    ESP_ERROR_CHECK(esp_event_handler_register(DEBOUNCE_EVENT,
        ESP_EVENT_ANY_ID,
        debounce_event_handler,
        nullptr));
    event_loop.handler_register<DEBOUNCE_EVENT>();

    gptimer_config_t config = 
    {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
        .flags
        {
            .intr_shared = true
        }
    };

    gptimer_alarm_config_t alarm_config =
    {
        .alarm_count = 10000, // period = 10ms @resolution 1MHz
        .reload_count = 0, // counter will reload with 0 on alarm event
        .flags
        {
            .auto_reload_on_alarm = true, // enable auto-reload
        }
    };

    ESP_LOGI(TAG, "phase 1: timer_service=%p, sizeof(Event)=%u, sizeof(app.q)=%u, sizeof(debouncers)=%u",
        &timer,
        sizeof(embr::debounce::v1::Event),
        sizeof(app.q),
        sizeof(debouncers));

    timer.start(&config, &alarm_config);

    init_gpio_input();

#ifdef CONFIG_DIAGNOSTIC_PERFMON
    // perfmon stuff lifted from components/perfmon/test/test_perfmon_ansi.c
    // NOTE: At this time I really don't know how to use this feature
    // See https://www2.lauterbach.com/pdf/debugger_xtensa.pdf
    // That doc says to look up "xtensa_debug_guide.pdf" but I can't find that one

    xtensa_perfmon_stop();
    xtensa_perfmon_init(0, 0, 0xffff, 0, 6);
    xtensa_perfmon_reset(0);
#endif

    using clock = estd::chrono::freertos_clock;

    clock::time_point last_now = clock::now();
    constexpr estd::chrono::seconds timeout(1);

    for(;;)
    {
        static int counter = 0;

        App::Event item;

        if(app.q.receive(&item, estd::chrono::milliseconds(10)))
            log(TAG, item);

        if(clock::now() - last_now > timeout)
        {
            last_now += timeout;
            ESP_LOGI(TAG, "counting: %d", ++counter);

#ifdef CONFIG_DIAGNOSTIC_PERFMON
            if(counter % 10 == 0)
            {
                xtensa_perfmon_dump();
                xtensa_perfmon_reset(0);
            }
#endif
        }
    }
}

