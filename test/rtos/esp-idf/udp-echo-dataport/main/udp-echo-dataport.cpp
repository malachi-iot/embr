#include <embr/platform/lwip/dataport.h>
#include <embr/dataport.hpp>
#include <embr/datapump.hpp>
#include <embr/observer.h>

#include "esp_log.h"

typedef embr::DataPump<embr::lwip::experimental::UdpDataportTransport> datapump_type;

struct AppObserver
{
    static constexpr const char* TAG = "AppObserver";

    template <class TDatapump>
    using event = embr::DataPortEvents<TDatapump>;

    template <class TDatapump, class TSubject>
    using DataPort = embr::DataPort<
        TDatapump, 
        typename TDatapump::transport_descriptor_t, 
        TSubject>;

    //template <class TDatapump> //, class TSubject>
    //void on_notify(typename event<TDatapump>::receive_dequeuing e)
    //template <class TSubject>
    // FIX: Though free form TContext compiles and functions, we don't
    // like it since it isn't reflecting strong typing the way it could be.
    template <class TContext>
    void on_notify(typename event<datapump_type>::receive_dequeuing e,
        //DataPort<datapump_type, TSubject>& context)
        //DataPort<TDatapump, TSubject>& dataport)
        TContext& context)
    {
        ESP_LOGI(TAG, "Got here");

        // FIX: unable to do this because apparently TContext is subject iself
        // and not the dataport as expected
        /*
        typedef typename TContext::ostreambuf_type ostreambuf_type;

        ostreambuf_type out;

        context.send(out, e.item.addr()); */
    }
};



void udp_echo_handler(void*)
{
    static constexpr const char* TAG = "udp_echo_handler";

    ESP_LOGI(TAG, "Start");

    AppObserver observer1;
    embr::lwip::DiagnosticDataportObserver<datapump_type> observer2;

    auto subject = embr::layer1::make_subject(
        observer2,
        observer1);
    // FIX: Doesn't work for some reason
    //auto subject = embr::layer1::make_subject(AppObserver());

    auto dataport = embr::lwip::make_udp_dataport(subject, 7);

    for(;;)
    {
        vTaskDelay(2);

        dataport.service();
    }
}