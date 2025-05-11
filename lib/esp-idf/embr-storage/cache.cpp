#include <esp_check.h>

#include "embr/esp-idf/nvs/cache.hpp"

static const char* TAG = "embr::storage";

namespace embr::esp_idf {

esp_err_t nvs_allocator_base::write_block(uint16_t block_number,
    uint16_t id, uint16_t size_in_blocks, bool with_erase)
{
    const uint32_t offset = offset_from_block(block_number);

    if(with_erase)  ESP_ERROR_CHECK(erase_range(0, sector_size));

    header h{size_in_blocks, id, true};

    esp_err_t ret;

    ret = write(offset, &h, sizeof(h));

    return ret;
}

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

        // For now, all tracking material must fit in one block
        assert(count * sizeof(control::descriptor) < minimum_usable);

        // TODO: Of course we need to heed count

        /*
        for(uint32_t o = offset + offsetof(header, descriptors);
         ; o += sizeof(control::descriptor))
        {
            control::descriptor d{0, 0xFF};

            ret = write(o, &d, sizeof(d));
        }   */
    }

    return ret;
}


unsigned nvs_allocator_base::header::find_candidate(
    uint16_t requested_size_in_blocks,
    unsigned total_blocks) const
{
    int candidate = -1;
    int candidate_count = 0;
    unsigned total_erase_counts = 0;
    int best_candidate = -1;

    for(int i = 0; i < total_blocks; ++i)
    {
        const control::descriptor& d = descriptors[i];
        bool free = true;

        // TODO: Need to interrogate sector to find out just how free he is
        //if(d.free)
        if(free)
        {
            if(candidate == -1)
            {
                candidate = i;
                candidate_count = 1;
                total_erase_counts = 0;
            }
            else
            {
                ++candidate_count;
                total_erase_counts += d.erased_total();
            }
        }
        else if(candidate != -1)
        {
        }

        if(candidate_count == total_blocks)
        {
            if(best_candidate == -1)
            {
                best_candidate = candidate;
            }
        }
    }
    return 0;
}

}
