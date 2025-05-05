#pragma once

//#include <freertos/FreeRTOS.h>
//#include <freertos/semphr.h>

#include <spi_flash_mmap.h>
#include <esp_partition.h>
//#include <wear_levelling.h>

#include <estd/port/freertos/mutex.h>

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
    static constexpr unsigned block_size = 256;
    static constexpr auto npos = (uint16_t) -1;

    static_assert(block_size < sector_size && sector_size % block_size == 0);

    /*
    struct subheader
    {
        uint16_t size_in_blocks;

    }   __attribute__((packed));    */


    struct header
    {
        uint16_t size_in_blocks;
        uint16_t id : 14;    // 0 = control table
        uint16_t allocated : 1;

        uint16_t size_in_bytes() const
        {
            return size_in_blocks * block_size;
        }

        uint16_t size_in_sectors() const
        {
            return size_in_blocks * (sector_size / block_size);
        }

        // array count = size_in_sectors
        uint8_t write_counter[];

        // DEBT: Perhaps add an aligned flag and align to 4 byte boundary
        void* payload()
        {
            return this + (offsetof(header, write_counter) + size_in_sectors());
        }

    }   __attribute__((packed));

    const esp_partition_t* partition_;
    const void* data_ {};
    //wl_handle_t* wl_handle_;
    mmap_handle_t mmap_handle_;
    estd::freertos::timed_mutex<true> data_lock_;
    //SemaphoreHandle_t data_lock_;

    void open()
    {
        partition_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "embr");
        //ESP_ERROR_CHECK(wl_mount(partition_, wl_handle_));
    }

    esp_err_t mmap(size_t offset, const void** data, esp_partition_mmap_handle_t* out_handle)
    {
        return esp_partition_mmap(partition_, offset, sector_size, ESP_PARTITION_MMAP_DATA, data, out_handle);
    }

    // NOTE: offset isn't truly needed and mainly used for debugging who locked mmap
    esp_err_t mmap(size_t offset)
    {
        if(!data_lock_.try_lock_for(estd::chrono::seconds(1)))
            return ESP_ERR_TIMEOUT;
        return mmap(offset, &data_, &mmap_handle_);
    }

    void munmap()
    {
        esp_partition_munmap(mmap_handle_);
        data_ = nullptr;
        data_lock_.unlock();
    }

    const header* data() const { return data_; } 

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
        uint16_t pos_in_blocks_;
        // DEBT: Side effect-y, for now
        //mmap_handle_t mmap_handle_;
        uint16_t pos() const { return pos_in_blocks_ * block_size; }

        const header* mmap()
        {
            //const void* data;
            //ESP_ERROR_CHECK(parent_->mmap(pos(), &data, &mmap_handle_));
            ESP_ERROR_CHECK(parent_->mmap(pos()));
            //auto data = (const uint8_t*) parent_->data_;
            //return (const header*) (data + pos());
            return parent_->data();
        }

        void munmap()
        {
            //esp_partition_munmap(mmap_handle_);
            parent_->munmap();
        }

        constexpr bool operator ==(const accessor& other) const
        {
            return pos_in_blocks_ == other.pos_in_blocks_;
        }
    };

    // TODO: Consider also a 'raw iterator' which moves forward by size, not by pointer

    struct iterator : accessor
    {
        iterator& operator++()
        {
            pos_in_blocks_ += mmap()->size_in_blocks;
            //pos_ = mmap()->next;
            munmap();
            return *this;
        }
    };

    struct end_iterator {};

    iterator begin()
    {
        iterator it;

        it.parent_ = this;
        it.pos_in_blocks_ = 0;

        return it;
    }

    // DEBT: Optimize me
    iterator end()
    {
        iterator it;

        it.parent_ = this;
        it.pos_in_blocks_ = npos;

        return it;
    }

    constexpr uint16_t size_in_blocks() const
    {
        return partition_->size / sector_size;
    }

    // NOTE: Best if you only doing this really for a brand new fresh partition
    // an erase is better if we're already formatted
    void format()
    {
        header h{1, 0, true};

        write(0, &h, sizeof(h));

        h = {static_cast<uint16_t>(size_in_blocks() - 1), 1, false};

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