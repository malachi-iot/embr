#pragma once

#include "../fwd.h"

namespace embr {

inline namespace property { inline namespace v1 {
namespace event { namespace internal {

template <class TOwner, class TValue = void, class enabled = void>
struct PropertyChanged;

template <class TOwner, class TValue>
struct PropertyChanged<TOwner, TValue, estd::enable_if_t<
    !estd::is_base_of<tag::property_traits, TOwner>::value> >
{
    typedef TOwner owner_type;
    typedef TValue value_type;

    owner_type* const owner;
    const value_type value;
};

template <class TOwner>
struct PropertyChanged<TOwner, void,
    estd::enable_if_t<
        !estd::is_base_of<tag::property_traits, TOwner>::value> >
{
    typedef TOwner owner_type;

    owner_type* const owner;
//protected:
    const int id_;

//public:
    int id() const { return id_; }
};

// DEBT: Fix naming and make this a specialization of PropertyChanged if we can
// FIX: Not using anyway because initializer list doesn't flow through to base class smoothly
template <class TTraits>
struct PropertyChangedTraits :
    PropertyChanged<typename TTraits::owner_type, typename TTraits::value_type>
{};

template <class TTraits>
struct PropertyChanged<TTraits, void,
    estd::enable_if_t<
        estd::is_base_of<tag::property_traits, TTraits>::value> >
{
    typedef typename TTraits::owner_type owner_type;
    typedef typename TTraits::value_type value_type;

    owner_type* const owner;
    const value_type value;
};


}}
}}

}