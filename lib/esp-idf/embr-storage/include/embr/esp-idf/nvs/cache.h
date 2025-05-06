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

namespace nvs {

struct partition
{
    static constexpr unsigned sector_size = SPI_FLASH_SEC_SIZE;

    const esp_partition_t* partition_;

    esp_err_t erase_range(size_t addr, size_t size) const
    {
        return esp_partition_erase_range(partition_, addr, size);
    }

    esp_err_t write(size_t addr, const void* src, size_t size) const
    {
        return esp_partition_write(partition_, addr, src, size);
    }

    esp_err_t read(size_t offset, void* dst, size_t size) const
    {
        return esp_partition_read(partition_, offset, dst, size);
    }
};

}

struct nvs_allocator_base : nvs::partition
{
    using mmap_handle_t = esp_partition_mmap_handle_t;

    static constexpr unsigned block_size = 256;
    static constexpr auto npos = (uint16_t) -1;

    static_assert(block_size < sector_size && sector_size % block_size == 0);

    /*
    struct subheader
    {
        uint16_t size_in_blocks;

    }   __attribute__((packed));    */


    // A null header indicates it and the remainder of the sector are null
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

        // DEBT: Really, we want to do this based off id, but in the short term
        // size_in_blocks will do
        constexpr bool null() const
        {
            return size_in_blocks == 0xFFFF;
        }

    }   __attribute__((packed));

    const void* data_ {};
    //wl_handle_t* wl_handle_;
    mmap_handle_t mmap_handle_;
    estd::freertos::timed_mutex<true> data_lock_;
    //SemaphoreHandle_t data_lock_;

    esp_err_t open()
    {
        partition_ = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "embr");
        //ESP_ERROR_CHECK(wl_mount(partition_, wl_handle_));

        // TODO: do esp_partition_verify

        return partition_ == nullptr ? ESP_ERR_NO_MEM : ESP_OK;
    }

    void close()
    {
        munmap();
        partition_ = nullptr;
    }

    // DEBT: Make him map entire partition size
    esp_err_t mmap(size_t offset, const void** data, esp_partition_mmap_handle_t* out_handle)
    {
        return esp_partition_mmap(partition_, offset, sector_size, ESP_PARTITION_MMAP_DATA, data, out_handle);
    }

    // NOTE: offset isn't truly needed and mainly used for debugging who locked mmap
    esp_err_t mmap(size_t offset, estd::chrono::freertos_clock::duration timeout = estd::chrono::seconds(1))
    {
        if(!data_lock_.try_lock_for(timeout))
            return ESP_ERR_TIMEOUT;
        return mmap(offset, &data_, &mmap_handle_);
    }

    void munmap()
    {
        // DEBT: May want another mutex
        if(data_ == nullptr)    return;

        esp_partition_munmap(mmap_handle_);
        data_ = nullptr;
        data_lock_.unlock();
    }

    const header* data() const { return (const header*)data_; } 

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

    // is_empty indicates whether it's already erased (all 0xFF), avoiding an additional
    // erase_range possibly
    bool is_formatted(bool* is_empty = nullptr)
    {
        header h;
        //read(0, )
        return false;
    }

    // NOTE: Best if you only doing this really for a brand new fresh partition
    // an erase is better if we're already formatted
    esp_err_t format(bool with_erase = true)
    {
        if(with_erase)  ESP_ERROR_CHECK(erase_range(0, sector_size));

        esp_err_t err;

        header h{1, 0, true};

        err = write(0, &h, sizeof(h));

        if(err != ESP_OK)   return err;

        h = {static_cast<uint16_t>(size_in_blocks() - 1), 1, false};

        return write(sector_size, &h, sizeof(h));
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