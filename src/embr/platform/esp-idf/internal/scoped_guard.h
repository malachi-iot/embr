#pragma once

#include <esp_err.h>
#include "../../../internal/scoped_guard.h"

namespace embr { namespace internal {

template <>
struct scoped_status_traits<esp_err_t>
{
    static bool good(esp_err_t e) { return e == ESP_OK; }
    inline static void log(esp_err_t e)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(e);
    }
    inline static void assert_(esp_err_t e)
    {
        ESP_ERROR_CHECK(e);
    }
};



}}