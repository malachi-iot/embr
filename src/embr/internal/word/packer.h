#pragma once

#include <estd/algorithm.h>
#include <estd/bit.h>

#include <estd/internal/macro/push.h>

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
#endif  // UNUSED

// Helper function to access the nth element of an array at compile time.
template <size_t N, typename T, size_t Size>
constexpr const T& get_element(const T (&arr)[Size])
{
    static_assert(N < Size, "Index out of bounds");
    return arr[N];
}

template <size_t N, typename T>
constexpr const T& get_element2(const T* arr)
{
    return arr[N];
}

template <size_t I, typename T>
constexpr const T& set_element(const T* in, T* out)
{
    return out[I] = in[I];
}

template <class T, size_t N>
struct array_helper;

template <class T, size_t N>
struct array_helper
{
    using base_type = array_helper<T, N - 1>;

    static constexpr T& copy(const T* in, T* out, T&&)
    {
        return base_type::copy(in + 1, out + 1, *out = *in);
    }
};

template <class T>
struct array_helper<T, 0>
{
    static constexpr T& copy(const T* in, T* out, T&&)
    {
        return *out = *in;
    }
};

template <size_t N, class T>
struct set_elements
{
    T array[N];

    template <size_t ...I>
    constexpr set_elements(const T* in, estd::index_sequence<I...>) :
        array{in[I]...}   {}

    constexpr set_elements(const T* in) :
        set_elements(in, estd::make_index_sequence<N>{}) {}
};

/*
template <size_t ...I, typename T>
constexpr const void set_elements(const T* in, T* out)
{
    return out[I] = in[I];
}*/

template <class T, size_t ...I>
void noloop_reverse_copy_helper(const T* in, T* out, estd::index_sequence<I...>)
{
    //set_element<I...>(in, out);
}

template <size_t N, class T>
constexpr T* noloop_reverse_copy(const T* in, T* out)
{
    return (new (out) set_elements<N, T>{in, estd::make_reverse_index_sequence<N>{}})->array;
    //noloop_reverse_copy_helper(in, out, make_reverse_integer_sequence<size_t, N>{});
}


// Just incase somehow hardcoding 0 speeds things up.  probably regular fill_n just fine
template <class ForwardIt, typename Size>
constexpr ForwardIt fill_zero_n(ForwardIt first, Size count)
{
    for(; count != 0; count--) *first++ = 0;

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
    static uint8_t* pack(const value_type& in, uint8_t* out)
    {
        // If N is higher precision than Integer, pad end of LE raw data
        if(N > sizeof(value_type))
        {
            fill_zero_n(out + sizeof(value_type), offset);
        }

        auto in_ptr = (const uint8_t*)&in;
        // DEBT: Make an estd reverse_copy
        noloop_reverse_copy<smallest_N>(in_ptr, out);
        //std::reverse_copy(in_ptr, in_ptr + smallest_N, out);
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
    static constexpr size_t offset = N > sizeof(value_type) ? (N - sizeof(value_type)) : 0;

    // in is little endian, and we are a little endian machine
    // out is big endian
    static void pack(value_type in, uint8_t* out)
    {
        // If N is higher precision than Integer, pad beginning of BE raw data
        if(N > sizeof(value_type))  out = fill_zero_n(out, offset);

        auto in_ptr = (uint8_t*)&in;
        noloop_reverse_copy<smallest_N>(in_ptr, out);
        //std::reverse_copy(in_ptr, in_ptr + smallest_N, out);
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
            noloop_reverse_copy<N - offset>(in + offset, (uint8_t*)&out);
            //std::reverse_copy(in + offset, in + N, (uint8_t*)&out);
        }
        else
        {
            // If Integer is more precise than N, or as precise as N
            noloop_reverse_copy<N>(in, (uint8_t*)&out);
            //std::reverse_copy(in, in + N, (uint8_t*)&out);
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

    // TODO: A constexpr-friendly version of this would be very useful

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

#include <estd/internal/macro/pop.h>
