# In the end, this optimization attempt saves almost no time.  Queuing it here for posterity

    // Retrieve when length is < 8, omits the run through more than 2 bytes
    template <typename TForwardIt, typename TInt>
    static inline void get_bytesize(descriptor d, TForwardIt raw, TInt& v)
    {
        typedef typename estd::iterator_traits<TForwardIt>::value_type byte_type;
        constexpr size_t byte_width = sizeof(byte_type) * byte_size();

        v >>= d.bitpos;

        // 'i' represents remaining bits after initial bitpos byte is processed
        int i = d.length - (byte_width - d.bitpos);

        auto remaining_bits = (byte_type)i;
        byte_type mask = (1 << remaining_bits) - 1;

        ++raw;

        v <<= remaining_bits;
        v |= (*raw & mask);
    }


