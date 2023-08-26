#pragma once

#include <esp_log.h>
#include <esp_mac.h>
#include <esp_crc.h>
#include <esp_check.h>

#include "esp-now.h"

#include "result.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

template <class TSubject, class TImpl>
auto EspNow::runtime<TSubject, TImpl>::on_start() -> state_result
{
    esp_err_t ret = ESP_FAIL;
    
    const uint8_t dummy_pmk[16] {};
    
    ESP_GOTO_ON_ERROR(esp_now_init(), exit, TAG, "init failed");
    ESP_GOTO_ON_ERROR(esp_now_register_send_cb(callback), exit, TAG, "register send callback failed");
    ESP_GOTO_ON_ERROR(esp_now_register_recv_cb(callback), exit, TAG, "register recv callback failed");

//#if CONFIG_ESP_WIFI_STA_DISCONNECTED_PM_ENABLE
//    ESP_ERROR_CHECK( esp_now_set_wake_window(65535) );
//#endif
    //configuring(&dummy_pmk);
    /* Set primary master key. */
    ESP_GOTO_ON_ERROR(esp_now_set_pmk(dummy_pmk), exit, TAG, "failed to set PMK");

exit:
    return create_start_result(ret);
}

template <class Subject, class Impl>
void EspNow::runtime<Subject, Impl>::callback(
    const esp_now_recv_info_t* info,
    const uint8_t* data,
    int len)
{
    static_type<Subject>::value->notify(event::receive{*info, {data, (unsigned)len}});
}

template <class Subject, class Impl>
void EspNow::runtime<Subject, Impl>::callback(
    const uint8_t* mac_addr,
    esp_now_send_status_t status)
{
    static_type<Subject>::value->notify(event::send{{mac_addr}, status});
}

}}
}