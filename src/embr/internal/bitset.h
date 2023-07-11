#include <estd/string.h>
#include <estd/type_traits.h>

#include "../word.h"

#if __cpp_exceptions
// DEBT: Since for optional and variant we already made our own versions, it's time
// we corralled them and "out_of_range" here
//#include <estd/stdexcept.h>

#include <stdexcept>
#endif

// DEBT: This ultimately is going to live in estd.  Placing here for now because
// low-level 'word' that bitset depends on also needs to move to estd, but hasn't yet

namespace estd {

// DEBT: We'll need to specialize for environments whose N exceeds maximum
// integer size
template <size_t N>
class bitset
{
    // Unsigned with no strictness enforcement since we are implicitly
    // strict by virtue of limited access via bitset API
    typedef embr::word<N, false, embr::word_strictness::none> word_type;

    word_type data_;
    using limits = numeric_limits<word_type>;

    static bool assert_in_range(size_t pos)
    {
#if __cpp_exceptions
        if(pos >= N) throw std::out_of_range("pos >= N");
#else
        if(pos >= N) abort();
#endif
        return {};
    }

public:
    ESTD_CPP_CONSTEXPR_RET bitset() : data_(0) {}

    // NOTE: This might violate strictness rules, but that appears to conform
    // to bitset standard in this case
    ESTD_CPP_CONSTEXPR_RET bitset(typename word_type::type val) :       // NOLINT
        data_(val)
    {}

    ESTD_CPP_CONSTEXPR_RET bool operator[](size_t pos) const
    {
        return data_.cvalue() >> pos & 1;
    }

    ESTD_CPP_CONSTEXPR_RET bool test(size_t pos) const
    {
        return (assert_in_range(pos), operator[](pos));
    }

    bitset& set(std::size_t pos)
    {
        assert_in_range(pos);
        data_ |= word_type(1 << pos);
        return *this;
    }

    bitset& reset(size_t pos)
    {
        assert_in_range(pos);
        data_ &= ~word_type(1 << pos);
        return *this;
    }

    bitset& set(size_t pos, bool value)
    {
        assert_in_range(pos);
        return value ? set(pos) : reset(pos);
    }

    template <size_t pos, enable_if_t<pos < N, bool> = true>
    bitset& set()
    {
        data_ |= word_type(1 << pos);
        return *this;
    }

    template <size_t pos, enable_if_t<pos < N, bool> = true>
    constexpr bool test() const
    {
        return (data_.cvalue() >> pos) & 1;
    }

    bitset& flip(std::size_t pos)
    {
        assert_in_range(pos);
        data_ ^= word_type(1 << pos);
        return *this;
    }

    bitset& flip()
    {
        data_ ^= word_type(limits::max());
        return *this;
    }

    constexpr bool all() const
    {
        //return data_ == word_type(limits::max());
        // DEBT: Need a word-specific version of this compare
        return data_ == limits::max();
    }

    constexpr bool any() const
    {
        return data_ != 0;
    }

    constexpr bool none() const
    {
        return data_ == 0;
    }

    constexpr size_t size() const
    {
        return N;
    }

    template <class TImpl>
    void to_string(estd::internal::dynamic_array<TImpl>& a, char zero = '0', char one = '1')
    {
        // DEBT: Better way to do this would be to preallocate N from a,
        // then index backwards via 'remaining'

        // DEBT: I *think* would be the underlying mechanism of resize, without the
        // specific assignment of characters.  resize itself doesn't exist in estd yet though
        //a.ensure_total_size(N);

        unsigned remaining = N;
        const typename word_type::type v = data_.value();
        typename word_type::type mask = 1 << (N - 1);

        while(remaining--)
        {
            // DEBT: underlying push_back can use some work
            // 1. don't call the underlying 'append' flavor
            // 2. rvalue version maybe should be a forward, not a move
            a.push_back((mask & v) ? one : zero);
            mask >>= 1;
        }
    }

    // DEBT: estd string has a flavor with a constant length - that's the one we want here
    // NOTE: Can't remember if layer1 string includes or excludes null term for size count, but
    // I think excludes.  Something has always felt a bit overly confusing in that
    // NOTE: Relies quite heavily on RVO
    estd::layer1::string<N + 1> to_string(char zero = '0', char one = '1')
    {
        estd::layer1::string<N + 1> s;
        to_string(s, zero, one);
        return s;
    }
};

}