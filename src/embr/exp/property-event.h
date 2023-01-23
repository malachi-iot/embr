#pragma once

#include "../observer.h"
#include "property-fwd.h"

namespace embr { namespace experimental {

namespace event {

// TODO: Consider changing this to property_traits_base
// and/or putting in a new 'properties' namespace
template <class TOwner, class T, int id_>
struct traits_base : traits_tag
{
    typedef TOwner owner_type;
    typedef T value_type;
    static constexpr int id() { return id_; }
    static constexpr const char* name() { return "N/A"; }
};

namespace internal {

template <class TOwner, class TValue = void, class enabled = void>
struct PropertyChanged;

template <class TOwner, class TValue>
struct PropertyChanged<TOwner, TValue, estd::enable_if_t<
    !estd::is_base_of<traits_tag, TOwner>::value> >
{
    typedef TOwner owner_type;
    typedef TValue value_type;

    owner_type* const owner;
    const value_type value;
};

template <class TOwner>
struct PropertyChanged<TOwner, void,
    estd::enable_if_t<
        !estd::is_base_of<traits_tag, TOwner>::value> >
{
    typedef TOwner owner_type;

    owner_type* const owner;
//protected:
    const int id_;

//public:
    const int id() { return id_; }
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
        estd::is_base_of<traits_tag, TTraits>::value> >
{
    typedef typename TTraits::owner_type owner_type;
    typedef typename TTraits::value_type value_type;

    owner_type* const owner;
    const value_type value;
};


}


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
            estd::is_base_of<traits_tag, TTraits>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<TTraits>& copy_from) :
        base_type{copy_from.owner, copy_from.id()},
        owner_name_(TTraits::owner_type::name())
    {}
};

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
            estd::is_base_of<traits_tag, TTraits>::value &&
            estd::is_same<typename TTraits::value_type, value_type>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<TTraits>& copy_from) :
        base_type{copy_from.owner, copy_from.value},
        id_{copy_from.id()}
    {}
};

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
    estd::is_base_of<traits_tag, TTraits>::value
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
    estd::is_base_of<traits_tag, TTraits>::value
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


template <typename TOwner, int id_>
struct PropertyChanged<TOwner, id_, typename estd::enable_if<
    estd::is_base_of<typename TOwner::id::lookup_tag, typename TOwner::id>::value
>::type> : PropertyTraits3<TOwner, id_>
    , internal::PropertyChanged< PropertyTraits3<TOwner, id_> >
{
    typedef internal::PropertyChanged< PropertyTraits3<TOwner, id_> > base_type;
    using typename base_type::value_type;

    PropertyChanged(TOwner* owner, value_type v) : base_type{owner, v} {}

    // For converting traits flavor to owner (this one) flavor
    template <class TTraits,
        typename estd::enable_if<
            estd::is_base_of<traits_tag, TTraits>::value &&
            estd::is_same<typename TTraits::owner_type, TOwner>::value &&
            estd::is_same<typename TTraits::value_type, value_type>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<TTraits>& copy_from) :
        base_type{copy_from.owner, copy_from.value}
    {}
};


template <typename TOwner>
struct PropertyChanged<TOwner, -1, typename estd::enable_if<
    estd::is_base_of<typename TOwner::id::lookup_tag, typename TOwner::id>::value
>::type> : internal::PropertyChanged<TOwner>
{
    typedef internal::PropertyChanged<TOwner> base_type;

    PropertyChanged(TOwner* owner, int v) : base_type{owner, v} {}

    template <int id>
    PropertyChanged(const PropertyChanged<TOwner, id>& copy_from) :
        base_type{copy_from.owner, copy_from.id()}
    {}
};


template <typename TOwner, int id_>
struct PropertyChanging<TOwner, id_, typename estd::enable_if<
    //estd::is_base_of<owner_tag, TOwner>::value
    estd::is_base_of<typename TOwner::id::lookup_tag, typename TOwner::id>::value
>::type> : PropertyTraits3<TOwner, id_>
{
    typedef PropertyTraits3<TOwner, id_> traits_type;
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