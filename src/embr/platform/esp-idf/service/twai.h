#pragma once

#include <embr/service.h>

#include <esp_log.h>

#include <driver/twai.h>

namespace embr::esp_idf {

namespace service { inline namespace v1 {

class TWAI : public embr::service::v1::Service
{
    using this_type = TWAI;
    using Service = embr::service::v1::Service;

protected:
    TaskHandle_t worker = nullptr;

public:
    static constexpr const char* TAG = "TWAI";
    static constexpr const char* name() { return TAG; }

    struct event
    {
        struct alert
        {
            const uint32_t alerts;

            operator uint32_t() const { return alerts; }
        };

        struct rx {};

        enum errors
        {
            ACTIVE,
            BUS,
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
            const twai_timing_config_t* t_config);

        void check_status();

        void start_task();

    private:
        static void worker__(void*);
        void worker_();

    // DEBT: Temporarily making this public as we work out the kinks
    public:
        void broadcast(uint32_t alerts);

    EMBR_SERVICE_RUNTIME_END

    // DEBT: Copy/pasting this everywhere sucks
    template <class TSubject>
    using static_type = static_factory<TSubject, this_type>::static_type;

    state_result on_stop();
};

}}

}
