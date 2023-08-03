#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// For ESP32->state result converter func
#include "result.h"

#include "twai.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

template <class TSubject, class TImpl>
auto TWAI::runtime<TSubject, TImpl>::on_start(
    const twai_general_config_t* g_config,
    const twai_timing_config_t* t_config,
    const twai_filter_config_t* f_config) -> state_result
{
    configuring(g_config);
    configuring(t_config);
    configuring(f_config);

    ESP_LOGD(TAG, "on_start: rx=%u tx=%u",
        g_config->rx_io,
        g_config->tx_io);

    esp_err_t r = twai_driver_install(g_config, t_config, f_config);

    configured(g_config);
    configured(t_config);
    configured(f_config);

    switch(r)
    {
        case ESP_ERR_INVALID_STATE:
        case ESP_ERR_INVALID_ARG:
            return { Error, ErrConfig };

        case ESP_ERR_NO_MEM:
            return { Error, ErrMemory };

        case ESP_OK:
        {
            r = twai_start();

            if(r != ESP_OK) return { Error, ErrConfig };

            //Prepare to trigger errors, reconfigure alerts to detect change in error state
            r = twai_reconfigure_alerts(
                TWAI_ALERT_ERR_ACTIVE |
                TWAI_ALERT_ABOVE_ERR_WARN | 
                // Have to disable this one, because logger goes crazy and WDT lights off
                TWAI_ALERT_BUS_ERROR |
                TWAI_ALERT_ERR_PASS | 
                TWAI_ALERT_BUS_OFF |
                TWAI_ALERT_BUS_RECOVERED |

                // DEBT: May want to enable this sometimes, consider making this 
                // a configuration point for service.  That said, one can call
                // twai_reconfigure_alerts any time to add this in at that point
                //TWAI_ALERT_AND_LOG |

                TWAI_ALERT_RX_DATA |
                TWAI_ALERT_RX_QUEUE_FULL |
                TWAI_ALERT_TX_FAILED
                , NULL);

            // DEBT: Check status and also flag this to be disabled if user
            // wishes to do the alert stuff themselves (or not at all)
            //start_task();
                
            return create_start_result(r);
        }

        default:
            return { Error, ErrUnspecified };
    }
}


template <class TSubject, class TImpl>
void TWAI::runtime<TSubject, TImpl>::check_status()
{
    twai_status_info_t twai_status;

    // DEBT: Don't do ESP_ERROR_CHECK, instead update service state
    ESP_ERROR_CHECK(twai_get_status_info(&twai_status));

    switch(twai_status.state)
    {
        case TWAI_STATE_RUNNING:
            state(Online);
            break;

        case TWAI_STATE_BUS_OFF:
            state(Offline);
            break;

        case TWAI_STATE_RECOVERING:
            state(Connecting);
            break;

        case TWAI_STATE_STOPPED:
        default:
            // *right* after a twai_start we expect anything but this.
            // if we get here, something badly went wrong
            state(Error);
            break;
    }
}


// TODO: Add this to overall embr/esp idf service Kconfig
#if CONFIG_MIOT_ENABLE_TWAI
template <class TSubject, class TImpl>
auto TWAI::runtime<TSubject, TImpl>::on_start(
    const twai_timing_config_t* t_config) -> state_result
{
    // DEBT: May want to put these static consts outside of class, there's
    // a chance compiler won't optimize away multiple flavors of them
    static twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CONFIG_MIOT_TWAI_TX_PIN,
        (gpio_num_t)CONFIG_MIOT_TWAI_RX_PIN,
        TWAI_MODE_NORMAL);
    static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // DEBT: Without this twai_driver_install flips out due to lack of interrupt 
    // availability (https://github.com/espressif/esp-idf/issues/7955)
    // clearing to 0 works, but I think results in esp-idf "hunting" for a new
    // interrrupt which may lead to unpredictability if OTHER interrupt-dependent
    // code is activated
    g_config.intr_flags = 0;

    return on_start(&g_config, t_config, &f_config);
}
#endif


template <class TSubject, class TImpl>
void TWAI::runtime<TSubject, TImpl>::broadcast(uint32_t alerts)
{
    ESP_LOGV(TAG, "broadcast: alerts=%" PRIx32, alerts);

    notify(event::alert{alerts});

    if(alerts & TWAI_ALERT_RX_DATA)
    {
        if(autorx())
        {
            twai_message_t message;

            while(twai_receive(&message, 0) == ESP_OK)
                notify(event::rx{&message});
        }
        else
        {
            notify(event::rx{});
        }
    }
    if(alerts & TWAI_ALERT_RX_QUEUE_FULL)
    {
        notify(event::error<event::RX>{});
    }
    if(alerts & TWAI_ALERT_TX_FAILED)
    {
        notify(event::error<event::TX>{});
    }
    if(alerts & TWAI_ALERT_ERR_ACTIVE)
    {
        notify(event::error<event::ACTIVE>{});
    }
    if(alerts & TWAI_ALERT_RECOVERY_IN_PROGRESS)
    {
        
    }
    if(alerts & TWAI_ALERT_BUS_RECOVERED)
    {
        
    }
    if(alerts & TWAI_ALERT_BUS_ERROR)
    {
        notify(event::error<event::BUS>{});
    }
    if(alerts & TWAI_ALERT_BUS_OFF)
    {
        
    }
    if(alerts & TWAI_ALERT_TX_RETRIED)
    {

    }
    if(alerts & TWAI_ALERT_TX_SUCCESS)
    {
        notify(event::tx{});
    }
}


template <class TSubject, class TImpl>
inline void TWAI::runtime<TSubject, TImpl>::worker_()
{
    ESP_LOGD(TAG, "worker: entry");

    for(;;)
    {
        uint32_t alerts;

        esp_err_t r = twai_read_alerts(&alerts, pdMS_TO_TICKS(500));

        switch(r)
        {
            case ESP_OK:
                broadcast(alerts);
                break;

            case ESP_ERR_TIMEOUT:
                //ESP_LOGV(TAG, "worker: timeout");
                break;

            default:
                state(Error, ErrConfig);
                break;
        }
    }
}


template <class TSubject, class TImpl>
void TWAI::runtime<TSubject, TImpl>::worker__(void* pvParameters)
{
    ((this_type*)pvParameters)->worker_();
}


template <class TSubject, class TImpl>
void TWAI::runtime<TSubject, TImpl>::start_task()
{
    TaskHandle_t* handle = &this->worker;

    BaseType_t r = xTaskCreate(worker__,
        "TWAI worker",
        4096,
        this,
        1,
        handle);
}


// DEBT: Put this off in a twai.cpp somewhere so that we don't have to specify
// inline
inline auto TWAI::on_stop() -> state_result
{
    // DEBT: Do actual service state adjustments here, instead of
    // error check
    ESP_ERROR_CHECK(twai_stop());
    ESP_ERROR_CHECK(twai_driver_uninstall());

    return { Stopped, Finished };
}

// DEBT: Consolidate this with check_status(), though alerts might be
// a kind of superset of it
template <class TSubject, class TImpl>
esp_err_t TWAI::runtime<TSubject, TImpl>::poll(TickType_t ticks_to_wait)
{
    uint32_t alerts;

    esp_err_t ret = twai_read_alerts(&alerts, ticks_to_wait);

    switch(ret)
    {
        case ESP_OK:
            broadcast(alerts);
            break;

        case ESP_ERR_TIMEOUT:
            break;

        default:
            state(Error, ErrConfig);
            break;
    }

    return ret;
}



}}

}
