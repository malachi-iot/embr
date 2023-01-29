#pragma once

#include "../../observer.h"

#include "traits.h"
#include "internal/event.h"

namespace embr {

inline namespace property { inline namespace v1 {

namespace event {

// 100% runtime flavor - that being the case, we can't practically provide 'value' itself
// Unless we do some tricky RTTI/forward cast magic which isn't ready for primetime AND
// this particular object needs more info than a compile time one anyway, making it not
// the best base class choice
// TODO: Not usable yet because no runtime identifier for owner has been decided upon,
// though leaning towards 'name'.  For that to work though we need some real honest to goodness
// global static const char []/* names
template <>
struct PropertyChanged<> : internal::PropertyChanged<void>
{
    typedef internal::PropertyChanged<void> base_type;

private:
    const char* owner_name_ = nullptr;

public:
    const char* owner() const { return owner_name_; }

    // For converting traits flavor to this one
    template <class TTraits,
        typename estd::enable_if<
            estd::is_base_of<tag::property_traits, TTraits>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<TTraits>& copy_from) :
        base_type{copy_from.owner, copy_from.id()},
        owner_name_(TTraits::owner_type::name())
    {}
};

// Pure enum variety
template <typename TEnum>
struct PropertyChanged<TEnum, -1, typename estd::enable_if<
    //!estd::is_base_of<traits_tag, T>::value &&
    //!estd::is_base_of<owner_tag, T>::value &&
    estd::is_enum<TEnum>::value
>::type> : internal::PropertyChanged<void, TEnum>
{
    typedef internal::PropertyChanged<void, TEnum> base_type;
    typedef TEnum value_type;

    const int id_;
    int id() const { return id_; }

    template <int id_>
    PropertyChanged(const PropertyChanged<value_type, id_>& copy_from) :
        base_type(copy_from.owner, copy_from.value),
        id_(id_)
    {}

    /*
    // NOTE: Won't work since traits_base isn't an EXACT match but rather a base class
    template <int id__>
    PropertyChanged(const PropertyChanged<traits_base<T, id__> >& copy_from) :
        id_{copy_from.id()},
        value(copy_from.value)
    {

    }
     */

    // For converting traits flavor to enum (this one) flavor
    template <class TTraits,
        typename estd::enable_if<
            estd::is_base_of<tag::property_traits, TTraits>::value &&
            estd::is_same<typename TTraits::value_type, value_type>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<TTraits>& copy_from) :
        base_type{copy_from.owner, copy_from.value},
        id_{copy_from.id()}
    {}
};

// "legacy" version, phasing this one out - too vague to use
template <typename TEnum, int id_>
struct PropertyChanged<TEnum, id_, typename estd::enable_if<
    //!estd::is_base_of<traits_tag, T>::value &&
    //!estd::is_base_of<owner_tag, T>::value &&
    estd::is_enum<TEnum>::value &&
    id_ >= 0>::type>
{
    const TEnum value;

    static constexpr int id() { return id_; }

    // TODO: This needs a converting constructor to bring it in from traits flavor w/ owner
    PropertyChanged(TEnum value) : value{value} {}
};

// Primary traits flavor - most 1:1 with how we manage properties
template <typename TTraits>
struct PropertyChanged<TTraits, -1, typename estd::enable_if<
    estd::is_base_of<tag::property_traits, TTraits>::value
>::type> :
    TTraits,
    internal::PropertyChanged<TTraits>
{
    typedef internal::PropertyChanged<TTraits> base_type;

    using typename base_type::value_type;
    using typename base_type::owner_type;

    PropertyChanged(owner_type* owner, value_type value) : base_type{owner, value}
    {}
};


template <typename TTraits>
struct PropertyChanging<TTraits, -1, typename estd::enable_if<
    estd::is_base_of<tag::property_traits, TTraits>::value
>::type> :
    TTraits
{
    typedef typename TTraits::value_type value_type;
    using typename TTraits::owner_type;

    owner_type& owner;
    const value_type old_value;
    const value_type new_value;

    PropertyChanging(owner_type& owner, value_type old, value_type new_value) :
        owner(owner),
        old_value{old},
        new_value{new_value}
    {}
};


// Owner variety with a specific property
template <typename TOwner, int id_>
struct PropertyChanged<TOwner, id_, typename estd::enable_if<
    estd::is_base_of<typename TOwner::id::property_owner, typename TOwner::id>::value
>::type> : PropertyTraits<TOwner, id_>
    , internal::PropertyChanged< PropertyTraits<TOwner, id_> >
{
    typedef internal::PropertyChanged< PropertyTraits<TOwner, id_> > base_type;
    using typename base_type::value_type;

    PropertyChanged(TOwner* owner, value_type v) : base_type{owner, v} {}

    // For converting traits flavor to owner (this one) flavor
    template <class TTraits,
        typename estd::enable_if<
            estd::is_base_of<tag::property_traits, TTraits>::value &&
            estd::is_same<typename TTraits::owner_type, TOwner>::value &&
            estd::is_same<typename TTraits::value_type, value_type>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<TTraits>& copy_from) :
        base_type{copy_from.owner, copy_from.value}
    {}
};


// Owner variety with any property
template <typename TOwner>
struct PropertyChanged<TOwner, -1, typename estd::enable_if<
    estd::is_base_of<typename TOwner::id::property_owner, typename TOwner::id>::value
>::type> : internal::PropertyChanged<TOwner>
{
    typedef internal::PropertyChanged<TOwner> base_type;

    PropertyChanged(TOwner* owner, int v) : base_type{owner, v} {}

    template <int id>
    PropertyChanged(const PropertyChanged<TOwner, id>& copy_from) :
        base_type{copy_from.owner, copy_from.id()}
    {}
};


// Owner variety with a specific property
template <typename TOwner, int id_>
struct PropertyChanging<TOwner, id_, typename estd::enable_if<
    //estd::is_base_of<owner_tag, TOwner>::value
    estd::is_base_of<typename TOwner::id::property_owner, typename TOwner::id>::value
>::type> : PropertyTraits<TOwner, id_>
{
    typedef PropertyTraits<TOwner, id_> traits_type;
    typedef typename traits_type::value_type value_type;

    TOwner* const owner;
    const value_type old_value;
    const value_type new_value;

    PropertyChanging(TOwner* owner, value_type old, value_type new_value) :
        owner(owner),
        old_value{old},
        new_value{new_value}
    {}
};



}

}}

}