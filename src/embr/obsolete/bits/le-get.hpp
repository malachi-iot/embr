// [2] 2.1.3.1.
template <>
struct getter<endianness::little_endian,
    length_direction::lsb_to_msb,
    resume_direction::lsb_to_msb
>
{
    typedef TInt int_type;

    template <class TForwardIt, typename TIntShadow = TInt,
        estd::enable_if_t<(sizeof(TIntShadow) > 1), bool> = true>
    inline static void get_assist(int& i, TForwardIt& raw, TInt& v)
    {
        constexpr unsigned byte_width = byte_size();

        // DEBT: turn i into unsigned and do an if statement above, rather than
        // forcing a typecast onto byte_width - though making byte_width an actual
        // signed int wouldn't be so bad
        for(; i > (int)byte_width; i -= byte_width)
        {
            --raw;
            v <<= byte_width;
            v |= *raw;
        }
    }

    // if TInt is never big enough to do bit shifting in the first place
    // (8-bit, basically) then don't even try.  Technically this is an
    // optimization, since the loop kicks out any actual attempt to bit shift,
    // but we also do this because clang complains at the theoretical
    // possibility of bitshifting an 8 bit value by 8 bits
    template <class TForwardIt, typename TIntShadow = TInt,
        estd::enable_if_t<(sizeof(TIntShadow) == 1), bool> = true>
    inline static void get_assist(int& i, TForwardIt& raw, TInt& v)
    {

    }

    template <class TReverseIt>
    static TInt get(const unsigned width, descriptor d, TReverseIt raw)
    {
        constexpr unsigned byte_width = byte_size();

        if(d.bitpos + d.length <= byte_width)
        {
            return getter<byte, no_endian, lsb_to_msb>::get(d, raw);
        }

        unsigned outside_bits = width - d.length;
        unsigned msb_outside_bits = outside_bits - d.bitpos;
        unsigned msb_inside_bits = byte_width - msb_outside_bits;

        byte msb_mask = (1 << msb_inside_bits) - 1;

        TInt v = *raw & msb_mask;

        int i = d.length - msb_inside_bits;

        get_assist(i, raw, v);

        /*
        // DEBT: turn i into unsigned and do an if statement above, rather than
        // forcing a typecast onto byte_width - though making byte_width an actual
        // signed int wouldn't be so bad
        for(; i > (int)byte_width; i -= byte_width)
        {
            --raw;
            v <<= 8;
            v |= *raw;
        } */

        // at this point, 'i' is 'lsb_inside_bits' which should be
        // the same as byte_width - d.bitpos

        --raw;

        /*
         * At assembly level, we roughly expect to see:
         * An additional branch/compare in exchange for a subtraction and extra shift
         * It may actually be worth it to still do this, but keeping commented until we
         * actually evaluate the underlying assembly
        if(d.bitpos == 0)
        {
            v <<= byte_width;
            v |= *raw;
        }
        else */
        {
            unsigned lsb_inside_bits = byte_width - d.bitpos;

            v <<= lsb_inside_bits;

            byte temp = *raw >> d.bitpos;

            v |= temp;
        }

        return v;
    }

    template <class TIt>
    inline static TInt get_adjusted(descriptor d, TIt raw)
    {
        unsigned width = width_deducer_lsb_to_msb(d);

        return get(width, d, raw + ((width - 1) / 8));
    }

    // EXPERIMENTAL
    template <unsigned bitpos, unsigned length, class TReverseIt>
    static TInt get(TReverseIt raw)
    {
        // TODO: Optimize
        return get(width_deducer_lsb_to_msb<bitpos, length>(), descriptor{bitpos, length}, raw);
    }

    // EXPERIMENTAL
    template <unsigned bitpos, unsigned length, class TIt>
    static TInt get_adjusted(TIt raw)
    {
        // TODO: Optimize
        constexpr unsigned width = width_deducer_lsb_to_msb<bitpos, length>();

        return get(width, descriptor{bitpos, length}, raw + ((width - 1) / 8));
    }
};