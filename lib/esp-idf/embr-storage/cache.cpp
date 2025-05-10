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


esp_err_t nvs_allocator_base::format_control(uint16_t block_number, header* copy_from, bool with_erase)
{
    esp_err_t ret = ESP_OK;

    const uint32_t offset = offset_from_block(block_number);

    if(copy_from)
    {
        mmap(offset);
        munmap();
    }
    else
    {
        // NOTE: Doesn't work with FEATURE_ERASE_COUNTER

        unsigned count = size_in_sectors();

        // TODO: Of course we need to heed count

        for(uint32_t o = offset + offsetof(header, descriptors);
         ; o += sizeof(control::descriptor))
        {
            control::descriptor d{0, 0xFF};

            ret = write(o, &d, sizeof(d));
        }
    }

    return ret;
}

}
