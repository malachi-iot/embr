#pragma once

#include <esp_wifi.h>

#include "event.h"

// As per:
// https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-guides/wifi.html
// https://github.com/espressif/esp-idf/blob/v5.0/components/esp_wifi/include/esp_wifi_types.h

namespace embr { namespace esp_idf { namespace event {

template <>
struct mapping<wifi_event_t, WIFI_EVENT_SCAN_DONE>
{
    typedef wifi_event_sta_scan_done_t type;
};


template <>
struct mapping<wifi_event_t, WIFI_EVENT_STA_CONNECTED>
{
    typedef wifi_event_sta_connected_t type;
};


template <>
struct mapping<wifi_event_t, WIFI_EVENT_STA_DISCONNECTED>
{
    typedef wifi_event_sta_disconnected_t type;
};


template <>
struct mapping<wifi_event_t, WIFI_EVENT_STA_AUTHMODE_CHANGE>
{
    typedef wifi_event_sta_authmode_change_t type;
};


template <>
struct mapping<wifi_event_t, WIFI_EVENT_STA_BSS_RSSI_LOW>
{
    typedef wifi_event_bss_rssi_low_t type;
};


template <wifi_event_t id>
using wifi = base<wifi_event_t, id>;



}}}
