#include <esp-helper.h>

#include <esp_log.h>

#include <embr/observer.h>
#include <embr/platform/lwip/iostream.h>

// DEBT: Put this all into unity testing

struct event1 {};

struct Context
{
    int stuff1, stuff2;
};

template <class TSubject>
void notify_from_int(TSubject& s)
{
    s.notify(int{5});
}

template <class TSubject>
void notify_from_event1(TSubject& s)
{
    s.notify(event1());
}


template <class TSubject>
void notify_from_event1_with_context(TSubject& s)
{
    Context c;
    s.notify(event1(), c);
}


template <class TSubject>
void notify_from_event1_with_ipbuf(TSubject& s)
{
    embr::lwip::Pbuf pbuf(32);
    s.notify(event1(), pbuf);
    // Stack usage:
    // Adding this one line jumps us over 64 threshold to 96 bytes
    embr::lwip::upgrading::ipbufstream i(pbuf);
    // Adding this one line jumps us to 128 bytes
    s.notify(event1(), i);
}


int allow_counter = 0;

struct Observer1
{
    static constexpr const char* TAG = "Observer1";

    static void on_notify(int val)
    {
        ESP_LOGI(TAG, "Got int notify");
    }

    static void on_notify(event1)
    {
        ESP_LOGI(TAG, "Got event1 notify");
    }

    static bool allow_notify(event1)
    {
        ++allow_counter;
        return true;
    }
};


struct Observer2
{
    static constexpr const char* TAG = "Observer2";

    static void on_notify(event1)
    {
        ESP_LOGI(TAG, "Got event1 notify");
    }

    static void on_notify(event1, Context&)
    {
        ESP_LOGI(TAG, "Got event1 notify w/ context");
    }
};



extern "C" void app_main()
{
    const char* TAG = "app_main";

    init_flash();

    ESP_LOGI(TAG, "Startup");

    auto s = embr::layer0::subject<Observer1, Observer2>();

    // Observer2 won't participate in this one
    notify_from_int(s);

    notify_from_event1(s);
    notify_from_event1_with_context(s);
    
    // remember, this one notifies twice
    notify_from_event1_with_ipbuf(s);

    ESP_LOGI(TAG, "allow_counter=%d", allow_counter);
}

