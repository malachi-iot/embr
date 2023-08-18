#pragma once

#include <esp_check.h>
#include <nvs_flash.h>

#include "flash.h"

#include "result.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

template <class TSubject, class TImpl>
auto Flash::runtime<TSubject, TImpl>::on_start() -> state_result
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        state(Configuring);

        ESP_GOTO_ON_ERROR(nvs_flash_erase(), done, TAG, "couldn't erase");

        ret = nvs_flash_init();
    }

done:
    return create_start_result(ret);
}

}}

}