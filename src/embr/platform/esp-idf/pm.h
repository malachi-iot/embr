#include <esp_pm.h>

#include "traits.h"

namespace embr { namespace esp_idf {

class pm_lock
{
    esp_pm_lock_handle_t lock_;
    
    typedef esp_pm_lock_handle_t native_type;

public:
    pm_lock() = default;
    pm_lock(native_type lock) : lock_{lock} {}

    esp_err_t create(esp_pm_lock_type_t type, int arg = 0, const char* name = nullptr)
    {
        return esp_pm_lock_create(type, arg, name, &lock_);
    }

    esp_err_t acquire()
    {
        return esp_pm_lock_acquire(lock_);
    }

    esp_err_t release()
    {
        return esp_pm_lock_release(lock_);
    }

    esp_err_t free()
    {
        return esp_pm_lock_delete(lock_);
    }

    operator native_type() const { return lock_; }
};


inline namespace internal {

#ifdef ESP_CHIP_ID_ESP32
template <>
struct pm_traits<chip_id::ESP32>
{
    typedef esp_pm_config_esp32_t config_type;
};
#endif


#ifdef ESP_CHIP_ID_ESP32H2
template <>
struct pm_traits<chip_id::ESP32H2>
{
    typedef esp_pm_config_esp32h2_t config_type;
};
#endif


#ifdef ESP_CHIP_ID_ESP32C2
template <>
struct pm_traits<chip_id::ESP32C2>
{
    typedef esp_pm_config_esp32c2_t config_type;
};
#endif


#ifdef ESP_CHIP_ID_ESP32C3
template <>
struct pm_traits<chip_id::ESP32C2>
{
    typedef esp_pm_config_esp32c2_t config_type;
};
#endif

}

}}