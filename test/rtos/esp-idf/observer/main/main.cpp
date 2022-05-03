#include <esp-helper.h>

#include <esp_log.h>

#include <embr/observer.h>

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
};

extern "C" void app_main()
{
    const char* TAG = "app_main";

    init_flash();

    ESP_LOGI(TAG, "Startup");

    auto s = embr::layer0::subject<Observer1>();

    notify_from_int(s);
    notify_from_event1(s);
    notify_from_event1_with_context(s);
}

