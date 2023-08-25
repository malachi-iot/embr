// DEBT: This much specificity for just this WiFi test is no good
// attempt to move this out to our board.h area
#include <esp_log.h>

#include <led_strip.h>

// Lifted from:
// https://github.com/espressif/idf-extra-components/blob/3c52bba8cc6431e58b5c04b22820a859e6d343e6/led_strip/examples/led_strip_rmt_ws2812/main/led_strip_rmt_ws2812_main.c

#ifdef CONFIG_BOARD_ESP32C3_DEVKITM_1
// GPIO assignment
#define LED_STRIP_BLINK_GPIO  8
// Numbers of the LED in the strip
#define LED_STRIP_LED_NUMBERS 1
// 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

static const char *TAG = "example";

led_strip_handle_t configure_led(void)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,   // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_NUMBERS,        // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags {
            .invert_out = false
        }                // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .flags {
            .with_dma = false,               // DMA feature is available on ESP target like ESP32-S3
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}

#endif