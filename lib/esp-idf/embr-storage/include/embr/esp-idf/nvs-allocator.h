#pragma once

#include <spi_flash_mmap.h>
#include <esp_partition.h>
//#include <wear_levelling.h>

namespace embr::esp_idf {

#if SPI_FLASH_SEC_SIZE != 4096
#error At this time we require a sector size of 4096
#endif

// https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/storage/partition.html
// https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/storage/wear-levelling.html

struct nvs_allocator_base
{
    using mmap_handle_t = esp_partition_mmap_handle_t;

    static constexpr unsigned sector_size = SPI_FLASH_SEC_SIZE;
    static constexpr auto npos = (uint16_t) -1;

    // Positions/sizes are in multiples of sector_size

    struct header
    {
        uint16_t next;  // npos = nullptr/end of ll
        uint16_t size;
        uint16_t id;    // 0 = control table
        uint16_t allocated : 1;
    }   __attribute__((packed));

    const esp_partition_t* partition_;
    //wl_handle_t* wl_handle_;

    void open()
    {
        partition_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "embr");
        //ESP_ERROR_CHECK(wl_mount(partition_, wl_handle_));
    }

    esp_err_t mmap(size_t offset, const void** data, esp_partition_mmap_handle_t* out_handle)
    {
        return esp_partition_mmap(partition_, offset, sector_size, ESP_PARTITION_MMAP_DATA, data, out_handle);
    }

    // DEBT: We're gonna find out soon why they keep these APIs separate I'm sure
    esp_err_t write(size_t addr, const void* src, size_t size)
    {
        ESP_ERROR_CHECK(esp_partition_erase_range(partition_, addr, size));
        return esp_partition_write(partition_, addr, src, size);
    }

    esp_err_t read(size_t offset, void* dst, size_t size) const
    {
        return esp_partition_read(partition_, offset, dst, size);
    }

    struct accessor
    {
        nvs_allocator_base* parent_;
        uint16_t pos_;  // x sector size
        // DEBT: Side effect-y, for now
        mmap_handle_t mmap_handle_;

        const header* mmap()
        {
            const void* data;
            ESP_ERROR_CHECK(parent_->mmap(pos_ * sector_size, &data, &mmap_handle_));
            return (const header*) data;
        }

        void munmap()
        {
            esp_partition_munmap(mmap_handle_);
        }

        constexpr bool operator ==(const accessor& other) const
        {
            return pos_ == other.pos_;
        }
    };

    // TODO: Consider also a 'raw iterator' which moves forward by size, not by pointer

    struct iterator : accessor
    {
        iterator& operator++()
        {
            pos_ = mmap()->next;
            munmap();
            return *this;
        }
    };

    struct end_iterator {};

    iterator begin()
    {
        iterator it;

        it.parent_ = this;
        it.pos_ = 0;

        return it;
    }

    // DEBT: Optimize me
    iterator end()
    {
        iterator it;

        it.parent_ = this;
        it.pos_ = npos;

        return it;
    }

    // NOTE: Best if you only doing this really for a brand new fresh partition
    // an erase is better if we're already formatted
    void format()
    {
        const uint16_t part_size_in_blocks = partition_->size / sector_size;
        header h{1, 1, 0, true};

        write(0, &h, sizeof(h));

        h = {npos, part_size_in_blocks, 1, false};

        write(sector_size, &h, sizeof(h));
    }

    // Deallocate everything except control block
    void erase()
    {
        //uint16_t part_size_in_blocks = partition_->size / sector_size;

        for(iterator it = begin(); it != end(); ++it)
        {

        }

        //while(it.pos)
    }
};

}