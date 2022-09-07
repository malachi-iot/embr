#pragma once

#include "../word.h"
#include "bits.h"
#include "../fwd/bits/word.h"

#include "../platform/guard-in.h"

namespace embr { namespace bits {

namespace internal {

template <class T, size_t bitness>
ESTD_CPP_CONSTEXPR_RET T& set_word_lsb_to_msb(T* const raw, const internal::descriptor_base<bitness>& d, T value)
{
    return (*raw &= ~internal::shifted_mask<T>(d)) |= value << d.bitpos;
}

template <class T, size_t bitness>
ESTD_CPP_CONSTEXPR_RET T get_word_lsb_to_msb(const T* const raw, const internal::descriptor_base<bitness>& d)
{
    return (*raw >> d.bitpos) & internal::mask<T>(d);
}

// NOTE: Putting out here because I am considering a specialized bool version (bitness = 1)
template <bool is_const, typename T>
class reference_base
{
    typedef internal::descriptor_base<sizeof(T) * byte_size()> descriptor_type;

    const descriptor_type d;
    typedef typename estd::conditional<is_const, const T*, T*>::type pointer;
    pointer const raw;

public:
    ESTD_CPP_CONSTEXPR_RET reference_base(descriptor_type d, pointer raw) : d(d), raw(raw)
    {}

#if __cplusplus >= 201103L
    reference_base(const reference_base&) = default;
#else
    reference_base(const reference_base& copy_from) :
        raw(copy_from.raw)
    {}
#endif

    ESTD_CPP_CONSTEXPR_RET T value() const
    {
        return get_word_lsb_to_msb(raw, d);
    }

    ESTD_CPP_CONSTEXPR_RET operator T() const { return value(); }

    reference_base operator=(T v)
    {
        set_word_lsb_to_msb(raw, d, v);
        return *this;
    }
};



template <unsigned bits>
class word<bits, lsb_to_msb> : public embr::word<bits>
{
    typedef embr::word<bits> base_type;

protected:
#ifdef __cpp_alias_templates
    using word_type = typename base_type::type;
#else
    typedef typename base_type::type word_type;
#endif

private:
    // DEBT: Somehow, &base_type::value_ flips out here.  Find out why
    inline word_type* raw() { return &this->value_; }
    ESTD_CPP_CONSTEXPR_RET const word_type* raw() const { return &this->value_; }

public:
    // DEBT: Make this and base embr::word hide their default constructor according to
    // a strictness flag setting (currently the unimplemented word_strictness::initialized)
    ESTD_CPP_CONSTEXPR_RET word() {}

    typedef internal::descriptor_base<sizeof(word_type) * byte_size()> descriptor_type;

    ESTD_CPP_CONSTEXPR_RET word(word_type v) : base_type(v) {}

    typedef reference_base<false, word_type> reference;
    typedef reference_base<true, word_type> const_reference;

    inline void set(descriptor_type d, word_type value)
    {
        set_word_lsb_to_msb(raw(), d, value);
    }

    ESTD_CPP_CONSTEXPR_RET word_type get(descriptor_type d) const
    {
        return get_word_lsb_to_msb(raw(), d);
    }

    // EXPERIMENTAL
#ifdef FEATURE_CPP_DEFAULT_TARGS
    template <unsigned bitpos, unsigned length = 1>
#else
    template <unsigned bitpos, unsigned length>
#endif
    ESTD_CPP_CONSTEXPR_RET embr::word<length> get() const
    {
        return experimental::bit_traits<bitpos, length>::template get<word_type, lsb_to_msb>(raw());
    }

    // EXPERIMENTAL
#ifdef FEATURE_CPP_DEFAULT_TARGS
    template <unsigned bitpos, unsigned length = 1>
#else
    template <unsigned bitpos, unsigned length>
#endif
    //inline void set(experimental::word<length> v) const
    inline unsigned set(word_type v)
    {
        return experimental::bit_traits<bitpos, length>::template set<word_type, lsb_to_msb>(raw(), v);
    }

    // EXPERIMENTAL
    template <class TBitTraits>
    ESTD_CPP_CONSTEXPR_RET embr::word<TBitTraits::length> get_exp() const
    {
        return TBitTraits::get(raw());
    }

    // EXPERIMENTAL
    template <class TBitTraits>
    inline void set_exp(embr::word<TBitTraits::length> v)
    {
        TBitTraits::set(raw(), v);
    }

    inline reference operator[](descriptor_type d)
    {
        return reference(d, raw());
    }

    ESTD_CPP_CONSTEXPR_RET const_reference operator[](descriptor_type d) const
    {
        return const_reference(d, raw());
    }
};

}

}}

#define EMBR_BITS_WORD_GETTER(name, bitpos, length) \
ESTD_CPP_CONSTEXPR_RET embr::word<length> name() const  \
{                                                   \
    return base_type::get<bitpos, length>();                \
}

#define EMBR_BITS_WORD_SETTER(name, bitpos, length) \
inline void name(typename base_type::word_type v)   \
{                                                   \
    base_type::set<bitpos, length>(v);              \
}

#define EMBR_BITS_WORD_PROPERTY(name, bitpos, length) \
EMBR_BITS_WORD_GETTER(name, bitpos, length)             \
EMBR_BITS_WORD_SETTER(name, bitpos, length)

#include "../platform/guard-out.h"