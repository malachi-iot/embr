#pragma once

#include <estd/algorithm.h>
#include <estd/bit.h>

namespace embr { namespace internal {
// DEBT: 'platform' really feels like it out to be 'source'
template <class T, size_t size, estd::endian target,
    estd::endian platform = estd::endian::native>
struct packer;

#if UNUSED
// NOTE: This flavor truncates on unpack
template <>
struct packer<uint16_t, 3, estd::endian::little, estd::endian::little>
{
    static void pack(uint16_t in, uint8_t* out)
    {
        out[0] = 0;
        out[1] = in >> 8;
        out[2] = in & 0xFF;
    }

    static uint16_t unpack(const uint8_t* in)
    {
        uint16_t v = in[1];
        v <<= 8;
        v |= in[0];
        return v;
    }
};

// NOTE: This flavor truncates on pack
template <>
struct packer<uint32_t, 3, estd::endian::little, estd::endian::little>
{
    static uint8_t* pack(uint32_t in, uint8_t* out)
    {
        out[0] = in >> 16 & 0xFF;
        out[1] = in >> 8 & 0xFF;
        out[2] = in & 0xFF;
        return out;
    }

    static constexpr uint32_t unpack(const uint8_t* in)
    {
        return in[0] << 16 | in[1] << 8 | in[2];
    }
};
#endif


// Just incase somehow hardcoding 0 speeds things up.  probably regular fill_n just fine
template <class ForwardIt, typename Size>
inline ForwardIt fill_zero_n(ForwardIt first, const Size& count)
{
    for(unsigned i = count; i != 0; i--) *first++ = 0;

    return first;
}


template <typename Integer, size_t N>
struct packer<Integer, N, estd::endian::little, estd::endian::big>
{
    using value_type = Integer;
    static constexpr size_t smallest_N = estd::min(N, sizeof(value_type));
    static constexpr size_t largest_N = estd::max(N, sizeof(value_type));

    // Only valid when N > sizeof(value_type)
    static constexpr size_t offset = N - sizeof(value_type);

    // in is big endian, and we are a big endian machine
    // out is little endian
    static uint8_t* pack(value_type in, uint8_t* out)
    {
        // If N is higher precision than Integer, pad end of LE raw data
        if(N > sizeof(value_type))
        {
            fill_zero_n(out + sizeof(value_type), offset);
        }

        auto in_ptr = (uint8_t*)&in;
        std::reverse_copy(in_ptr, in_ptr + smallest_N, out);
        return out;
    }
};


template <typename Integer, size_t N>
struct packer<Integer, N, estd::endian::big, estd::endian::little>
{
    using value_type = Integer;
    static constexpr size_t smallest_N = estd::min(N, sizeof(value_type));
    static constexpr size_t largest_N = estd::max(N, sizeof(value_type));

    // Only valid when N > sizeof(value_type)
    static constexpr size_t offset = N - sizeof(value_type);

    // in is little endian, and we are a little endian machine
    // out is big endian
    static uint8_t* pack(value_type in, uint8_t* out)
    {
        // If N is higher precision than Integer, pad beginning of BE raw data
        if(N > sizeof(value_type))  out = fill_zero_n(out, offset);

        auto in_ptr = (uint8_t*)&in;
        std::reverse_copy(in_ptr, in_ptr + smallest_N, out);
        return out;
    }

    // in is big endian
    // out is little endian, and we are a little endian machine
    static value_type unpack(const uint8_t* in)
    {
        value_type out;

        if(N < sizeof(value_type))
            out = 0;

        if(N > sizeof(value_type))
        {
            // If Integer is less precise than N
            std::reverse_copy(in + offset, in + N, (uint8_t*)&out);
        }
        else
        {
            // If Integer is more precise than N, or as precise as N
            std::reverse_copy(in, in + N, (uint8_t*)&out);
        }

        return out;
    }
};

template <class Integer, size_t N>
struct packer<Integer, N, estd::endian::big, estd::endian::big>
{
    using value_type = Integer;
    static constexpr size_t smallest_N = estd::min(N, sizeof(value_type));
    static constexpr size_t largest_N = estd::max(N, sizeof(value_type));

    static uint8_t* pack(value_type in, uint8_t* out)
    {
        estd::copy_n(((uint8_t*)&in), smallest_N, out);
        if(N > sizeof(value_type))
        {
            for(unsigned i = 0; i < N - sizeof(value_type); i++)
                out[i] = 0;
        }

        return out;
    }
};

// NOTE: This flavor truncates on on pack
template <typename Integer, size_t N>
struct packer<Integer, N, estd::endian::little, estd::endian::little>
{
    using value_type = Integer;
    static constexpr size_t smallest_N = estd::min(N, sizeof(value_type));
    static constexpr size_t largest_N = estd::max(N, sizeof(value_type));

    static uint8_t* pack(value_type in, uint8_t* out)
    {
        /*
        out[0] = in >> 40;
        out[1] = in >> 32;
        out[2] = in >> 24;
        out[3] = in >> 16;
        out[4] = in >> 8;
        out[5] = in;*/
        // native flavor, we can do pure byte copies and pointer tricks
        estd::copy_n(((uint8_t*)&in), smallest_N, out);
        if(N > sizeof(value_type))
        {
            for(unsigned i = sizeof(value_type); i < N; i++)
                out[i] = 0;
        }

        return out;
    }

    static value_type unpack(const uint8_t* in)
    {
        value_type out;

        if(N < sizeof(value_type))  out = 0;

        estd::copy_n(in, N, (uint8_t*)&out);
        return out;
    }

    static constexpr uint64_t unpack_constexpr(const uint8_t* in)
    {
        // Could do this with copy_n in theory, but that would require some fancy casting
        // and trickier to make constexpr
        return uint64_t(in[5]) << 40 | uint64_t(in[4]) << 32 | in[3] << 24 | in[2] << 16 | in[1] << 8 | in[0];
    }
};

}}
