#pragma once

#include <esp_partition.h>
#include <wear_levelling.h>

namespace embr::esp_idf {

// https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/storage/partition.html
// https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/storage/wear-levelling.html

struct nvs_allocator_base
{

};

}