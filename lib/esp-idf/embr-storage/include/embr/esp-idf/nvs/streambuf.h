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

    void sync()
    {
        const unsigned offset = offset_ * sector_size;
        esp_partition_erase_range(partition_, offset, sector_size);
        esp_partition_write(partition_, offset, base_type::pbase(), sector_size);
    }

protected:
    int_type overflow(int_type ch = traits_type::eof())
    {
        return ch;
    }

public:
    ESTD_CPP_FORWARDING_CTOR(ostreambuf)
};

}
