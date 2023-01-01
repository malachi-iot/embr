#include "unity.h"

#include <esp_log.h>

#include <embr/platform/esp-idf/timer.h>
#include <embr/platform/esp-idf/app_format.h>
#include <embr/platform/esp-idf/pm.h>

#include <embr/internal/scoped_guard.h>

using namespace embr::esp_idf;


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




template <>
struct scoped_guard_traits<embr::esp_idf::pm_lock>
{
    typedef esp_err_t status_type;
    //typedef scoped_status_traits<esp_err_t> status_traits;
};


template <scoped_guard_fail_action fail_action>
class scoped_guard<embr::esp_idf::pm_lock, fail_action> :
    public scoped_guard_base<embr::esp_idf::pm_lock, fail_action != SCOPED_GUARD_ASSERT, fail_action>
{
    typedef scoped_guard_base<embr::esp_idf::pm_lock, fail_action != SCOPED_GUARD_ASSERT, fail_action> base_type;

public:
    scoped_guard(esp_pm_lock_type_t type, int arg = 0, const char* name = nullptr)
    {
        base_type::status(base_type::value().create(type, arg, name));
    }

    ~scoped_guard()
    {
        base_type::status(base_type::value().free());
    }
};


}}


template <esp_chip_id_t>
struct pm_traits;

template <>
struct pm_traits<ESP_CHIP_ID_ESP32>
{
    typedef esp_pm_config_esp32_t config_type;
};


#ifdef ESP_CHIP_ID_ESP32H2
template <>
struct pm_traits<ESP_CHIP_ID_ESP32H2>
{
    typedef esp_pm_config_esp32h2_t config_type;
};
#endif


#ifdef ESP_CHIP_ID_ESP32C2
template <>
struct pm_traits<ESP_CHIP_ID_ESP32C2>
{
    typedef esp_pm_config_esp32c2_t config_type;
};
#endif

// For esp_clk_cpu_freq
#include "esp_private/esp_clk.h"

// Guidance from https://github.com/espressif/esp-idf/blob/v5.0/examples/wifi/power_save/main/power_save.c


TEST_CASE("power management", "[esp_pm]")
{
    static const char* TAG = "esp_pm test";

    typedef chip_traits<chip_id()> chip_traits;

    ESP_LOGI(TAG, "chip_id=%s", chip_traits::name());

    embr::internal::scoped_guard<pm_lock, embr::internal::SCOPED_GUARD_SILENT> pml(ESP_PM_NO_LIGHT_SLEEP);

#if CONFIG_PM_ENABLE
    typedef pm_traits<chip_id()> pm_traits;

    //int cur_freq_mhz = esp_clk_cpu_freq() / MHZ;
    //int xtal_freq_mhz = esp_clk_xtal_freq() / MHZ;
    //int min_mhz = estd::min(cur_freq_mhz, xtal_freq_mhz);

    // DEBT: Just getting something in here to compile

    int max_mhz = 160;
    int min_mhz = 20;

    pm_traits::config_type config =
    {
        .max_freq_mhz = max_mhz,
        .min_freq_mhz = min_mhz,
#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
        .light_sleep_enable = true
#else
        .light_sleep_enable = false
#endif
    };

    ESP_ERROR_CHECK(esp_pm_configure(&config));

    TEST_ASSERT_EQUAL(ESP_OK, pml.status());
    TEST_ASSERT_TRUE(pml.good());
    
    pml->acquire();
    pml->release();
#else
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_SUPPORTED, pml.status());
#endif
}

