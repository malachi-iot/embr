#include "../esp-helper-platform.h"

extern "C" {

#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <esp_event_loop.h>

}

namespace embr { namespace experimental { namespace esp_idf {

template <int32_t id>
struct wifi_event;

namespace events {

namespace ip {

template <int32_t id>
struct base;

#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
template <>
struct base<IP_EVENT_STA_GOT_IP> : ip_event_got_ip_t
{
    base(const ip_event_got_ip_t& copy_from) :
        ip_event_got_ip_t(copy_from) {}
};

typedef base<IP_EVENT_STA_GOT_IP> got_ip;
#else
template <>
struct base<SYSTEM_EVENT_STA_GOT_IP> 
    : system_event_sta_got_ip_t 
{
    base(const system_event_sta_got_ip_t& copy_from) : 
        system_event_sta_got_ip_t(copy_from) {}
};

typedef base<SYSTEM_EVENT_STA_GOT_IP> got_ip;
#endif

}

}


template <class TSubject, class TContext>
static void _event_handler(TSubject& subject, TContext& context)
{
}

template <class TSubject, class TContext>
static void _event_handler(TContext& context)
{
    static TSubject subject;

    _event_handler(subject, context);
}

template <class TSubject, class TContext>
static void _event_handler()
{
    static TContext context;

    _event_handler<TSubject>(context);
}

// NOTE: This isn't a feature of default event loop.  It's a feature of the revised
// event system present in v4.0
// We call it FEATURE_IDF_DEFAULT_EVENT_LOOP since esp-idf confusingly refers to
// old one as "legacy", but only from 4.0 forward
// TODO: See if we can specialize and not even do static TSubject for truly
// stateless observers
#ifdef FEATURE_IDF_DEFAULT_EVENT_LOOP
template <class TSubject, class TContext>
static void event_handler(void* arg, 
    esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    TContext* context = (TContext*) arg;
    // Limited to a semi-singleton instance of a subject
    static TSubject subject;

    // UNTESTED
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_START:
            {
                break;
            }

            default:
                break;
        }
    }
    else if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
            {
                auto data = (ip_event_got_ip_t*) event_data;
                events::ip::got_ip e(*data);
                //data->ip_info.ip

                // TODO: Optimize this by putting in a separate event_handler function
                if(context == nullptr)
                    subject.notify(e);
                else
                    subject.notify(e, *context);

                break;
            }

            default:
                break;
        }
    }
}
#else
template <class TSubject, class TContext>
esp_err_t event_handler(void* ctx, system_event_t* event)
{
    TContext* context = (TContext*) ctx;
    static TSubject subject;
    
    switch(event->event_id)
    {
        case SYSTEM_EVENT_STA_START:
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
        {
            subject.notify(events::ip::got_ip(event->event_info.got_ip), *context);
            break;
        }

        case SYSTEM_EVENT_STA_DISCONNECTED:
            break;

        default:
            break;
    }

    return ESP_OK;
}
#endif

}}}

