#include <esp-helper.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_wifi.h"

#include <estd/chrono.h>
#include <estd/optional.h>
#include <estd/thread.h>

// DEBT: Just for testing right now, not yet using it
#include <embr/platform/esp-idf/adc.h>

#include <embr/platform/esp-idf/service/diagnostic.h>
#include <embr/platform/esp-idf/service/gpio.hpp>
#include <embr/platform/esp-idf/service/pm.hpp>

using Diagnostic = embr::esp_idf::service::v1::Diagnostic;

#include "app.h"


namespace app_domain {

App app;

typedef embr::layer0::subject<Diagnostic, singleton> filter_observer;

App::ADC::runtime<top_tier> adc;

}


extern int isr_counter;


extern "C" void app_main()
{
    const char* TAG = "app_main";

    ESP_LOGI(TAG, "startup: ADC service size=%u",
        sizeof(app_domain::adc));

    unsigned long counter = 0;
    constexpr static unsigned long thinner = SOC_ADC_SAMPLE_FREQ_THRES_LOW / 100;

    app_domain::app.start();

    for(;;)
    {
        App::io frame;
        App::io::pointer sample = nullptr;
        bool got_data = false;

        //if(app_domain::app.q.receive(&frame, estd::chrono::milliseconds(10)))
        if(app_domain::app.q.receive(&frame))
        {
            sample = frame.begin;
            got_data = true;
        }

        if(counter % thinner == 0)
        {
            if(sample == nullptr)
            {
                ESP_LOGI(TAG, "counting: %lu (%u)", counter / thinner, got_data);
            }
            else
            {
                ESP_LOGI(TAG, "counting: %lu, sample.data=%x",
                    counter / thinner,
                    (unsigned)sample->data());
            }

            ESP_LOGD(TAG, "isr_counter: %d", isr_counter);
        }

        ++counter;
    }
}

