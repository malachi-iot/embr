#pragma once

#include <estd/streambuf.h>

#include "fwd.h"

namespace embr::esp_idf::nvs::impl {

// Watch out for https://github.com/malachi-iot/estdlib/issues/103
class ostreambuf :
    public estd::internal::impl::out_span_streambuf<char, SPI_FLASH_SEC_SIZE>
{
    static constexpr unsigned sector_size = SPI_FLASH_SEC_SIZE;

    using base_type = estd::internal::impl::out_span_streambuf<char, sector_size>;
    using typename base_type::int_type;
    using typename base_type::traits_type;

    // multiples of sector_size, intra-sector position maintained by out_span base
    // NOTE: Optimization idea is to use an absolute 32-bit pos and modulo out the sector offset
    uint16_t offset_{};
    const esp_partition_t* partition_;

    int sync()
    {
        const unsigned offset = offset_ * sector_size;
        esp_err_t ret;
        ret = esp_partition_erase_range(partition_, offset, sector_size);
        if(ret != ESP_OK) return -1;
        esp_partition_write(partition_, offset, base_type::pbase(), sector_size);
        return ret == ESP_OK ? 0 : -1;
    }

    // amount of buffer space left we can write to
    constexpr int_type xout_avail() const { return sector_size - this->pos(); }

protected:
    int_type overflow(int_type ch = traits_type::eof())
    {
        return ch;
    }

public:
    ESTD_CPP_FORWARDING_CTOR(ostreambuf)
};

}
