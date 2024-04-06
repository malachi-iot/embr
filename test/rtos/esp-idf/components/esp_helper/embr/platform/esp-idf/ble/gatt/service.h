#pragma once

#include <esp_gatts_api.h>

#include "v1/service.h"

//namespace embr::esp_idf { inline namespace ble {

// Bleh, ADL didn't pick this up
// DEBT: Inlining this is bad
inline const char* to_string(esp_gatts_cb_event_t e, const char* na)
{
    switch(e)
    {
        case ESP_GATTS_REG_EVT:         return "REG";
        case ESP_GATTS_DISCONNECT_EVT:  return "DISCONNECT";
        case ESP_GATTS_CONNECT_EVT:     return "CONNECT";
        case ESP_GATTS_ADD_CHAR_EVT:    return "ADD_CHAR";
        case ESP_GATTS_MTU_EVT:         return "MTU";
        case ESP_GATTS_CLOSE_EVT:       return "CLOSE";
        case ESP_GATTS_READ_EVT:        return "READ";
        case ESP_GATTS_WRITE_EVT:       return "WRITE";
        case ESP_GATTS_CANCEL_OPEN_EVT: return "CANCEL_OPEN";
        case ESP_GATTS_CREATE_EVT:      return "CREATE";
        case ESP_GATTS_OPEN_EVT:        return "OPEN";
        case ESP_GATTS_LISTEN_EVT:      return "LISTEN";
        case ESP_GATTS_CONF_EVT:        return "CONF";
        case ESP_GATTS_CONGEST_EVT:     return "CONGEST";

        case ESP_GATTS_START_EVT:       return "START";
        case ESP_GATTS_STOP_EVT:        return "STOP";

        case ESP_GATTS_ADD_CHAR_DESCR_EVT:      return "ADD_CHAR_DESCR";
        case ESP_GATTS_SET_ATTR_VAL_EVT:        return "SET_ATTR_VAL";
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:      return "CREATE_ATTR_TAB";

        default: return na;
    }
}

//}}