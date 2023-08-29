#pragma once

#include <estd/port/freertos/queue.h>

#include <embr/platform/esp-idf/log.h>
#include <embr/platform/esp-idf/service/ledc.h>

namespace service = embr::esp_idf::service::v1;

struct App
{
    static constexpr const char* TAG = "App";

    void IRAM_ATTR on_notify(service::LEDControl::event::callback e);
};