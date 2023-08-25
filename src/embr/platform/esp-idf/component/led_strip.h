#pragma once

#include <led_strip.h>

namespace embr { namespace esp_idf {

struct led_strip
{
    led_strip_handle_t handle_;

    struct color
    {
        uint32_t red;
        uint32_t green;
        uint32_t blue;
    };

    esp_err_t set_pixel(int index, uint32_t r, uint32_t g, uint32_t b)
    {
        return led_strip_set_pixel(handle_, index, r, g, b);
    }

    esp_err_t refresh()
    {
        return led_strip_refresh(handle_);
    }

    esp_err_t clear()
    {
        return led_string_clear(handle_);
    }

    led_strip& operator=(led_strip_handle_t handle)
    {
        handle_ = handle;
        return *this;
    }

    operator led_strip_handle_t() const { return handle_; }
};

}}