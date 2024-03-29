

// [1] 2.1.3.1.
template <>
struct setter<endianness::little_endian,
    length_direction::lsb_to_msb,
    resume_direction::lsb_to_msb
>
{
    template <class TForwardIt, typename TIntShadow = TInt,
        estd::enable_if_t<(sizeof(TIntShadow) > 1), bool> = true>
    inline static void set_assist(unsigned& i, TForwardIt& raw, TInt& v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = byte_size();

        for(; i > byte_width; i -= byte_width)
        {
            *raw++ = (byte_type) v;
            v >>= byte_width;
        }
    }

    template <class TForwardIt, typename TIntShadow = TInt,
        estd::enable_if_t<(sizeof(TIntShadow) == 1), bool> = true>
    inline static
    (unsigned& i, TForwardIt& raw, TInt& v)
    {

    }

    // on bitpos=0 boundary
    // DEBT: It sure seems like the bigger, badder set can handle this without concerningly extra logic
    template <class TForwardIt>
    static void set(const unsigned width, unsigned v_length, TForwardIt raw, TInt v)
    {
        // DEBT: theoretically, byte_type might differ from embr::bits::byte - so
        // check for that
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = byte_size();
        unsigned outside_material = width - v_length;
        unsigned outside_right_material = outside_material;
        unsigned inside_right_material = byte_width - outside_right_material;

        //unsigned v_size = width_deducer2(v);

        byte_type
            // mask for last 'raw', for LE that is loosely MSB
            right_mask = (1 << inside_right_material) - 1;

        set_assist(v_length, raw, v);
        /*
        for(int i = v_length; i > byte_width; i -= byte_width)
        {
            *raw++ = (byte_type) v;
            v >>= 8;
        } */

        // only applicable when v_length is not a multiple of byte_width.  Otherwise
        // it's just a slightly more expensive = assignment
        *raw &= ~right_mask;
        *raw |= v;
    }

    ///
    /// @tparam TForwardIt
    /// @param width bit width, on byte boundaries, of underlying data store for 'raw'
    /// @param d
    /// @param raw
    /// @param v
    /// @remarks (width - d.length) must be <= 8 otherwise behavior is undefined
    template <class TForwardIt>
    static void set(const unsigned width, descriptor d, TForwardIt raw, TInt v)
    {
        constexpr size_t byte_width = byte_size();

        if(d.bitpos + d.length <= byte_width)
        {
            setter<byte, no_endian, lsb_to_msb>::set(d, raw, v);
            return;
        }

        if(d.bitpos == 0)
        {
            set(width, d.length, raw, v);
            return;
        }

        // DEBT: theoretically, byte_type might differ from embr::bits::byte - so
        // check for that
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        unsigned outside_material = width - d.length;
        unsigned outside_right_material = outside_material - d.bitpos;
        unsigned inside_right_material = byte_width - outside_right_material;

        unsigned v_size = width_deducer2(v);

        // NOTE: May actually not be necessary, since trailing zeroes for LE are like leading zeroes
        // for BE
        if(v_size >= d.length)
        {
            // incoming v's value is smaller than length allocated, so we may have to skip some
            // 'raw' bytes first since we're doing this forwards and not in reverse

            // come up with fake leading zeroes somehow
        }

        //size_t shift = 0;
        byte_type
            // mask for first 'raw', for LE that is loosely LSB
            left_mask = (1 << d.bitpos) - 1,
            // mask for last 'raw', for LE that is loosely MSB
            right_mask = (1 << inside_right_material) - 1;

        // first, smallest part of value is written, shifted over by bit position
        *raw &= left_mask;
        *raw |= (byte_type) (v << d.bitpos);

        unsigned inside_left_material = byte_width - d.bitpos;

        v >>= inside_left_material;

        // i = remaining bits to store, now that LSB on left side is written
        // since we check for sub-byte d.length at the top, there is no danger of
        // rollunder
        unsigned i = d.length - inside_left_material;

        set_assist(i, ++raw, v);

        /*
        for(; i > byte_width; i -= byte_width)
        {
            *++raw = (byte_type) v;
            v >>= 8;
        }

        // DEBT: This part feels sloppy
        if(i > 0) ++raw; */

        *raw &= ~right_mask;
        *raw |= v;
    }

    template <class TForwardIt>
    inline static void set(descriptor d, TForwardIt raw, TInt v)
    {
        unsigned width = width_deducer_lsb_to_msb(d);
        return set(width, d, raw, v);
    }

    template <class TForwardIt>
    static inline void set(TForwardIt raw, TInt v)
    {
        constexpr unsigned width = sizeof(TInt) * byte_size();

        set(width, width, raw, v);
    }

    // EXPERIMENTAL
    template <unsigned bitpos, unsigned length, class TForwardIt>
    static inline void set(TForwardIt raw, TInt v)
    {
        constexpr unsigned width = width_deducer_lsb_to_msb<bitpos, length>();

        set(width, descriptor{bitpos, length}, raw, v);
    }

    // EXPERIMENTAL
    template <class TReverseIt>
    static void set2(const unsigned width, descriptor d, TReverseIt raw, TInt v)
    {
        typedef typename estd::iterator_traits<TReverseIt>::value_type byte_type;
        constexpr size_t byte_width = byte_size();
        constexpr size_t max_int_width = sizeof(TInt) * byte_width;
        unsigned outside_material = width - d.length;
        unsigned outside_right_material = outside_material - d.bitpos;
        unsigned inside_right_material = byte_width - outside_right_material;

        size_t shifter = max_int_width - byte_width;

        while(shifter > byte_width)
        {
            byte_type _v = v >> shifter;
            shifter -= byte_width;
        }
    }
};