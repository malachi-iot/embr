#include "internal/scoped_guard.h"

#include <nvs_flash.h>
#include <nvs.h>

#include <esp_log.h>

// NOTE: esp-idf has its own C++ thing going on with full unique_ptr and virtual
// methods.  We're going a different toute
namespace embr { namespace esp_idf { namespace nvs {

// NOTE: Since they have varieties of handles in their c++ implementation, 
// I expect we'll need some template tricks or namespace segregation.  Not doing that now though.
struct Handle
{
    nvs_handle_t h;

public:
    esp_err_t open(const char* namespace_name, nvs_open_mode_t open_mode)
    {
        return nvs_open(namespace_name, open_mode, &h);
    }

    esp_err_t get_blob(const char* key, void* out_value, std::size_t* sz)
    {
        return nvs_get_blob(h, key, out_value, sz);
    }

    esp_err_t get_str(const char* key, char* out_value, std::size_t* sz)
    {
        return nvs_get_str(h, key, out_value, sz);
    }

    esp_err_t get(const char* key, uint8_t* out_value)
    {
        return nvs_get_u8(h, key, out_value);
    }

    esp_err_t set_blob(const char* key, const void* buffer, std::size_t sz)
    {
        return nvs_set_blob(h, key, buffer, sz);
    }

    esp_err_t set(const char* key, uint8_t value)
    {
        return nvs_set_u8(h, key, value);
    }

    esp_err_t set(const char* key, int8_t value)
    {
        return nvs_set_i8(h, key, value);
    }

    esp_err_t set(const char* key, uint16_t value)
    {
        return nvs_set_u16(h, key, value);
    }

    void close()
    {
        nvs_close(h);
    }

    operator nvs_handle_t() const { return h; }
};


template <class T>
esp_err_t get(Handle h, const char* key, T* blob)
{
    static const char* TAG = "nvs::get<TBlob>";

    std::size_t sz = sizeof(T);
    esp_err_t e;

    if((e = h.get_blob(key, blob, &sz)) != ESP_OK)
        return e;

    if(sz != sizeof(T)) 
    {
        e = ESP_ERR_INVALID_SIZE;
        ESP_LOGW(TAG, "uh oh!  load had a problem, sizes don't match");
    }

    return e;
}


template <class T>
esp_err_t set(Handle h, const char* key, T* blob)
{
    constexpr std::size_t sz = sizeof(T);

    return h.set_blob(key, blob, sz);
}



}}}
