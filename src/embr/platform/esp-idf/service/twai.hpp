#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// For ESP32->state result converter func
#include "result.h"

#include "twai.h"

// Takes a little extra time and memory to notice unwanted behaviors
#if !FEATURE_EMBR_SERVICE_TWAI_SANITY_CHECK
#define FEATURE_EMBR_SERVICE_TWAI_SANITY_CHECK 1
#endif


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

#if !CONFIG_TWAI_TIMING
    ESP_LOGD(TAG, "on_start: rx=%u tx=%u",
        g_config->rx_io,
        g_config->tx_io);
#endif

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

            // DEBT: Add in configuration point to disable/enable some of these.  Or,
            // ideally, detect if any observers even care in the first place

            //Prepare to trigger errors, reconfigure alerts to detect change in error state
            r = twai_reconfigure_alerts(
                TWAI_ALERT_ERR_ACTIVE |
                TWAI_ALERT_ABOVE_ERR_WARN | 
                // Have to disable this one, because logger goes crazy and WDT lights off
                TWAI_ALERT_BUS_ERROR |
                TWAI_ALERT_ERR_PASS | 
                TWAI_ALERT_BUS_OFF |
                TWAI_ALERT_RECOVERY_IN_PROGRESS |
                TWAI_ALERT_BUS_RECOVERED |
                TWAI_ALERT_PERIPH_RESET |

                // DEBT: May want to enable this sometimes, consider making this 
                // a configuration point for service.  That said, one can call
                // twai_reconfigure_alerts any time to add this in at that point
                //TWAI_ALERT_AND_LOG |

                TWAI_ALERT_RX_DATA |
                TWAI_ALERT_RX_QUEUE_FULL |
                TWAI_ALERT_RX_FIFO_OVERRUN |
                TWAI_ALERT_TX_RETRIED |
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
auto TWAI::runtime<TSubject, TImpl>::on_start(
    const twai_general_config_t* g_config,
    const twai_timing_config_t* t_config) -> state_result
{
    constexpr twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    return on_start(g_config, t_config, &f_config);
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


#if CONFIG_TWAI_TIMING
template <class TSubject, class TImpl>
auto TWAI::runtime<TSubject, TImpl>::on_start() -> state_result
{
    // DEBT: May want to put these static consts outside of class, there's
    // a chance compiler won't optimize away multiple flavors of them
    static twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CONFIG_GPIO_TWAI_TX,
        (gpio_num_t)CONFIG_GPIO_TWAI_RX,
#if defined(CONFIG_TWAI_MODE_NORMAL)
        TWAI_MODE_NORMAL);
#elif defined(CONFIG_TWAI_MODE_LISTEN_ONLY)
        TWAI_MODE_LISTEN_ONLY);
#else
        0);
#error Unknown mode config
#endif

    static constexpr twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    static constexpr twai_timing_config_t t_config =
#if CONFIG_TWAI_TIMING == 16
        TWAI_TIMING_CONFIG_16KBITS();
#elif CONFIG_TWAI_TIMING == 25
        TWAI_TIMING_CONFIG_25KBITS();
#elif CONFIG_TWAI_TIMING == 50
        TWAI_TIMING_CONFIG_50KBITS();
#elif CONFIG_TWAI_TIMING == 100
        TWAI_TIMING_CONFIG_100KBITS();
#elif CONFIG_TWAI_TIMING == 125
        TWAI_TIMING_CONFIG_125KBITS();
#elif CONFIG_TWAI_TIMING == 250
        TWAI_TIMING_CONFIG_250KBITS();
#elif CONFIG_TWAI_TIMING == 500
        TWAI_TIMING_CONFIG_500KBITS();
#elif CONFIG_TWAI_TIMING == 800
        TWAI_TIMING_CONFIG_800KBITS();
#elif CONFIG_TWAI_TIMING == 1000
        TWAI_TIMING_CONFIG_1MBITS();
#else
#error Unsupported TWAI timing
#endif

    ESP_LOGD(TAG, "on_start rx=%u, tx=%u, speed=%uKbit",
        CONFIG_GPIO_TWAI_TX,
        CONFIG_GPIO_TWAI_RX,
        CONFIG_TWAI_TIMING);

    // DEBT: Without this twai_driver_install flips out due to lack of interrupt 
    // availability (https://github.com/espressif/esp-idf/issues/7955)
    // clearing to 0 works, but I think results in esp-idf "hunting" for a new
    // interrrupt which may lead to unpredictability if OTHER interrupt-dependent
    // code is activated
    g_config.intr_flags = 0;

    return on_start(&g_config, &t_config, &f_config);
}
#else
template <class TSubject, class TImpl>
auto TWAI::runtime<TSubject, TImpl>::on_start() -> state_result
{
    static_assert(sizeof(TSubject) < 0, "CONFIG_TWAI_TIMING must be configured");
    return {};
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

#if FEATURE_EMBR_SERVICE_TWAI_SANITY_CHECK
            int sanity_check = 20;
#endif

            while(twai_receive(&message, 0) == ESP_OK)
            {
#if FEATURE_EMBR_SERVICE_TWAI_SANITY_CHECK
                // Observed twai_receive go into an "infinite receive" mode when 3 nodes
                // where terminated but only two were powered.  Doesn't reliably give a
                // TWAI_ALERT_BUS_ERROR, and even when it does we still infinitely receive.
                if(--sanity_check == 0)
                {
                    ESP_LOGW(TAG, "Getting too many consecutive reads.  Is the bus over-terminated?");
                    break;
                }
#endif
                notify(event::autorx{message});
            }
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
        notify(event::error<event::OFF>{});
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
void TWAI::runtime<TSubject, TImpl>::update_state(uint32_t alerts)
{
    // TODO: Do same thing as check_status here, but via alerts
    // NOTE: Observed are TWAI_ALERT_BUS_ERROR alerts with no compensating recovery
    // alert, just immediately followed by non-err alerts like TWAI_ALERT_RX_DATA
}


#if !CONFIG_TWAI_WORKER_POLL_MS
#define CONFIG_TWAI_WORKER_POLL_MS 500
#endif

template <class TSubject, class TImpl>
inline void TWAI::runtime<TSubject, TImpl>::worker_()
{
    ESP_LOGD(TAG, "worker: entry - polling period: %u",
        CONFIG_TWAI_WORKER_POLL_MS);

    // DEBT: Consider using xTaskNotify / xTaskNotifyWait, though
    // a simple flag is nice because service doesn't have to track entire task handle

    // Might also be interesting to do so to wake up/sleep this task when in an
    // unstarted state, or perhaps vTaskSuspend

    // NOTE: This task really requires no cleanup, so a direct vTaskDelete is reasonable
    // too

    constexpr unsigned timeout = pdMS_TO_TICKS(CONFIG_TWAI_WORKER_POLL_MS);

    while(signal_task_shutdown() == false)
    {
        uint32_t alerts;

        esp_err_t r = twai_read_alerts(&alerts, timeout);

        switch(r)
        {
            case ESP_OK:
                broadcast(alerts);
                break;

            case ESP_ERR_TIMEOUT:
                //ESP_LOGV(TAG, "worker: timeout");
                break;

            case ESP_ERR_INVALID_ARG:
                // Invalid argument here doesn't bring down the whole service per se,
                // but you can expect things aren't gonna work right
                if(state() == Started)  state(Degraded);
                break;

            case ESP_ERR_INVALID_STATE:
            default:
                vTaskDelay(timeout);
                //ESP_LOGV(TAG, "worker: error getting TWAI alert");
                // DEBT: Something feels a little off about setting error over and over again
                // on overall service
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

// DEBT: Add task priority, stack size and perhaps affinity

template <class TSubject, class TImpl>
inline BaseType_t TWAI::runtime<TSubject, TImpl>::start_task()
{
#if FEATURE_EMBR_SERVICE_TWAI_TASK
    TaskHandle_t* handle = &this->worker;
#else
    TaskHandle_t storage;
    TaskHandle_t* handle = &storage;
#endif

    BaseType_t r = xTaskCreate(worker__,
        "TWAI worker",
        4096,
        this,
        1,
        handle);

    return r;
}


// DEBT: Put this off in a twai.cpp somewhere so that we don't have to specify
// inline
// NOTE: Untested
inline auto TWAI::on_stop() -> state_result
{
#if FEATURE_EMBR_SERVICE_TWAI_TASK_SOFT_SHUTDOWN
    signal_task_shutdown(true);
#elif FEATURE_EMBR_SERVICE_TWAI_TASK
    if(worker != nullptr)   vTaskDelete(worker);
#else
    signal_task_shutdown(true);
#endif

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
            update_state(alerts);
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
