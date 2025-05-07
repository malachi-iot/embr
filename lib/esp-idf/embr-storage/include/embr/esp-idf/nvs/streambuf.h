#pragma once

#include <estd/streambuf.h>

#include "fwd.h"

namespace embr::esp_idf::nvs::impl {

// Watch out for https://github.com/malachi-iot/estdlib/issues/103
template <class CharTraits = estd::char_traits<char>, bool buffered = false>
class ostreambuf :
    // span is a neat idea, but do we really wanna burn 4k of RAM?
    //public estd::internal::impl::out_span_streambuf<char, SPI_FLASH_SEC_SIZE>
    public estd::internal::impl::out_pos_streambuf_base<CharTraits>
{
    static constexpr unsigned sector_size = SPI_FLASH_SEC_SIZE;

    //using base_type = estd::internal::impl::out_span_streambuf<char, sector_size>;
    using base_type = estd::internal::impl::out_pos_streambuf_base<CharTraits>;

public:
    using typename base_type::traits_type;
    using typename base_type::char_type;
    using typename base_type::int_type;

    using base_type::pbump;
    using base_type::pos;

private:
    using streamsize = estd::streamsize;

    // multiples of sector_size, intra-sector position maintained by out_span base
    // NOTE: Optimization idea is to use an absolute 32-bit pos and modulo out the sector offset
    uint16_t offset_{};
    const esp_partition_t* partition_;

    /* keep around in case we ever want a more sophisticated buffered version
    int sync()
    {
        const unsigned offset = offset_ * sector_size;
        esp_err_t ret;
        ret = esp_partition_erase_range(partition_, offset, sector_size);
        if(ret != ESP_OK) return -1;
        esp_partition_write(partition_, offset, base_type::pbase(), sector_size);
        return ret == ESP_OK ? 0 : -1;
    }   */

    constexpr uint32_t offset() const
    {
        return offset_ * sector_size + pos();
    } 

    // amount of buffer space left we can write to
    constexpr int_type xout_avail() const { return sector_size - pos(); }

protected:
    int_type overflow(int_type ch = traits_type::eof())
    {
        const char_type c = traits_type::to_char_type(ch);
 
        esp_err_t ret = esp_partition_write(partition_, offset(), &c, sizeof(char_type));

        pbump(1);

        return ch;
    }

public:
    ESTD_CPP_FORWARDING_CTOR(ostreambuf)

    streamsize xsputn(const char_type* s, streamsize count)
    {
        esp_err_t ret = esp_partition_write(partition_, offset(), s, count);

        pbump(count);

        return count;
    }
};

}

namespace embr::esp_idf::nvs {

template <class CharTraits>
using basic_ostreambuf = estd::internal::streambuf<impl::ostreambuf<CharTraits>>;

using ostreambuf = basic_ostreambuf<estd::char_traits<char>>;

}
