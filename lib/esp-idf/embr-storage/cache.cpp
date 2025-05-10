#include <esp_check.h>

#include "embr/esp-idf/nvs/cache.hpp"

static const char* TAG = "embr::storage";

namespace embr::esp_idf {

esp_err_t nvs_allocator_base::format_sector(uint16_t sector, uint16_t id, uint32_t size_in_bytes, bool with_erase)
{
    esp_err_t ret = ESP_OK;

    const uint32_t offset = sector * sector_size;
    uint16_t size_in_blocks = (size_in_bytes + block_size) / block_size;
    header h{size_in_blocks, id, true};

    if(with_erase)
        ESP_GOTO_ON_ERROR(
            erase_range(offset, sector_size),
            exit, TAG, "couldn't erase");

    ret = write(offset, &h, sizeof(h));

exit:
    return ret;
}

}
