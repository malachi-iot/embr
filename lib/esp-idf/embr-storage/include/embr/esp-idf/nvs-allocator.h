#pragma once

#include <esp_partition.h>
//#include <wear_levelling.h>

namespace embr::esp_idf {

// https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/storage/partition.html
// https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/storage/wear-levelling.html

struct nvs_allocator_base
{
    struct header
    {
        uint32_t next;
        uint32_t size;
    };

    const esp_partition_t* partition_;
    //wl_handle_t* wl_handle_;

    void open()
    {
        partition_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "embr");
        //ESP_ERROR_CHECK(wl_mount(partition_, wl_handle_));
    }

    esp_err_t mmap(const void** data, esp_partition_mmap_handle_t* out_handle)
    {
        return esp_partition_mmap(partition_, 0, 4096, ESP_PARTITION_MMAP_DATA, data, out_handle);
    }

    void write(size_t addr, const void* src, size_t size)
    {

    }
};

}