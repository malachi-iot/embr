#pragma once

#include <estd/chrono.h>

#include <embr/service.h>

#include <esp_log.h>

#include <driver/twai.h>

namespace embr::esp_idf {

namespace service { inline namespace v1 {

// TWAI service provides a start_task in all cases, but will only
// track the task if this flag is set (defaults to true)
#if !FEATURE_EMBR_SERVICE_TWAI_TASK
#define FEATURE_EMBR_SERVICE_TWAI_TASK 1
#endif

// TWAI service normally kills task via vTaskDelete
// set this flag to instead only shut down task based on
// signal flag
#if !FEATURE_EMBR_SERVICE_TWAI_TASK_SOFT_SHUTDOWN
#define FEATURE_EMBR_SERVICE_TWAI_TASK_SOFT_SHUTDOWN 0
#endif

class TWAI : public embr::service::v1::Service
{
    using this_type = TWAI;
    using Service = embr::service::v1::Service;

protected:
#if FEATURE_EMBR_SERVICE_TWAI_TASK
    TaskHandle_t worker = nullptr;
#endif

    static constexpr const unsigned OPTION_AUTORX = 0x01;

public:
    static constexpr const char* TAG = "TWAI";
    static constexpr const char* name() { return TAG; }

    struct event
    {
        // DEBT: Consider making alert a propery, as it kind of
        // meshes equally with that as an event
        struct alert
        {
            const uint32_t alerts;

            operator uint32_t() const { return alerts; }

            constexpr bool rx_data() const
            {
                return alerts & TWAI_ALERT_RX_DATA;
            }

            constexpr bool tx_failed() const
            {
                return alerts & TWAI_ALERT_TX_FAILED;
            }
        };

        struct rx {};

        // DEBT: rx vs autorx unrefined and might lead to confusion.  Review
        // used if autorx() = true
        struct autorx
        {
            const twai_message_t& message;
        };

        // tx succeeded
        struct tx {};

        // DEBT: Not always errors... statuses instead?
        enum errors
        {
            ACTIVE,
            BUS,    // bus error
            OFF,    // bus-off
            RX,
            TX,
            PASSIVE,
            WARN
        };

        template <errors E>
        using error = estd::integral_constant<errors, E>;
    };

    EMBR_SERVICE_RUNTIME_BEGIN(Service)

        state_result on_start(
            const twai_general_config_t* g_config,
            const twai_timing_config_t* t_config,
            const twai_filter_config_t* f_config);

        state_result on_start(
            const twai_general_config_t* g_config,
            const twai_timing_config_t* t_config);

        state_result on_start();

        void check_status();
        esp_err_t poll(TickType_t ticks_to_wait);

        // NOTE: Not ready yet
        template <class Period, class Rep>
        esp_err_t poll(const estd::chrono::duration<Period, Rep>& timeout);

        BaseType_t start_task();

        void autorx(bool v)
        {
            base_type::state_.user3.v1 = v;
        }

        bool autorx() const
        {
            return base_type::state_.user3.v1;
        }

        bool signal_task_shutdown() const
        {
            return base_type::state_.user3.v2;
        }

    private:
        constexpr bool signal_task_shutdown(bool v)
        {
            return base_type::state_.user3.v2 = v;
        }

        static void worker__(void*);
        void worker_();
        void broadcast(uint32_t alerts);
        void update_state(uint32_t alerts);

    EMBR_SERVICE_RUNTIME_END

    // DEBT: Copy/pasting this everywhere sucks
    template <class TSubject>
    using static_type = static_factory<TSubject, this_type>::static_type;

    state_result on_stop();
};

}}

}
