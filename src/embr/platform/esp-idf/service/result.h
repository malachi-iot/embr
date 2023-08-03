#pragma once

#include <embr/service.h>

namespace embr::esp_idf {

namespace service { inline namespace v1 {

constexpr embr::service::v1::Service::state_result create_start_result(esp_err_t ret)
{
    using s = embr::service::v1::Service;
    using state_result = s::state_result;

    switch(ret)
    {
        case ESP_OK:            return state_result::started();
        case ESP_ERR_TIMEOUT:   return { s::Error, s::ErrTimeout };
        case ESP_ERR_NO_MEM:    return { s::Error, s::ErrMemory };
        
        case ESP_ERR_NOT_FOUND:
        case ESP_ERR_NOT_SUPPORTED:
        case ESP_ERR_INVALID_ARG:
        case ESP_ERR_INVALID_STATE:
            return { s::Error, s::ErrConfig };
        
        default:                return { s::Error, s::ErrUnspecified };
    }
}


}}

}
