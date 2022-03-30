#include <embr/platform/lwip/dataport.h>
#include <embr/dataport.hpp>
#include <embr/datapump.hpp>
#include <embr/observer.h>

#include "esp_log.h"

#include "../../udp-echo/main/process.h"

typedef embr::DataPump<embr::lwip::experimental::UdpDataportTransport> datapump_type;
typedef struct pbuf* pbuf_pointer;

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
        ESP_LOGI(TAG, "on_notify: entry");

        typedef datapump_type::transport_descriptor_t::istreambuf_type istreambuf_type;
        typedef datapump_type::transport_descriptor_t::ostreambuf_type ostreambuf_type;

#if UNUSED
        //istreambuf_type in(*e.item.netbuf());
        //ostreambuf_type out(128);

        //estd::internal::basic_istream<istreambuf_type> _in(in);
#endif

        estd::internal::basic_istream<istreambuf_type> _in(*e.item.netbuf());
        {
            estd::internal::basic_ostream<ostreambuf_type> _out(128);

            pbuf_pointer pbuf = e.item.netbuf()->pbuf();

            ESP_LOGD(TAG, "pbuf in tot_len=%d, ref=%d", pbuf->tot_len, pbuf->ref);

            pbuf = _out.rdbuf()->pbuf();

            ESP_LOGD(TAG, "pbuf out tot_len=%d, ref=%d", pbuf->tot_len, pbuf->ref);

            // FIX: Doing process_out results in a stack overflow crash, and if
            // it doesn't crash, xsputn dies at memcpy or during 'ignore'.  Note also
            // that bumping up stack size doesn't always make a difference
            process_out(_in, _out);

            ESP_LOGD(TAG, "on_notify: phase 1");
        }

        // FIX: Although this works, it prematurely moves e.item.netbuf -
        // In theory other observers might want to pick it up
        /*
        context.enqueue_for_send(
            std::move(*e.item.netbuf()),
            e.item.addr()); */

        // FIX: unable to do this because apparently TContext is DatapumpSubject
        // and not the dataport as expected
        /*
        typedef typename TContext::ostreambuf_type ostreambuf_type;

        ostreambuf_type out;

        context.send(out, e.item.addr()); */

        ESP_LOGI(TAG, "on_notify: exit");
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