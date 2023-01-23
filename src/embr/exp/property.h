#pragma once

#include "../observer.h"

#define EMBR_INTERNAL_PROPERTY_TRAITS_BODY(owner, type, id, desc) \
typedef event::traits_base<owner, type, id> base_type; \
typedef owner owner_type;                              \
using typename base_type::value_type;                  \
static constexpr const char* name() { return desc; }


#define EMBR_PROPERTY_TRAITS_GETTER_SETTER(host_, name_) \
    static constexpr value_type get(host_& o)       \
    { return o.name_; }                                   \
    static inline void set(host_& o, value_type v)   \
    { o.name_ = v; }

#define EMBR_PROPERTY_ID_LOOKUP(name, id_)  \
template <bool dummy> struct lookup<id_, dummy> : name {}

#define EMBR_PROPERTY_ID_ALIAS(name_, id_, alias, desc) \
struct alias : event::traits_base<this_type, decltype(this_type::name_), id_>         \
{                                                          \
    EMBR_INTERNAL_PROPERTY_TRAITS_BODY(this_type, decltype(this_type::name_), id_, desc) \
    EMBR_PROPERTY_TRAITS_GETTER_SETTER(owner_type, name_)  \
    static inline owner_type& host(owner_type& o) { return o; } \
};  \
EMBR_PROPERTY_ID_LOOKUP(alias, id_);

#define EMBR_PROPERTY_ID(name, id, desc) EMBR_PROPERTY_ID_ALIAS(name, this_type::id, name, desc)
#define EMBR_PROPERTY_SPARSE_ID(name, type, id_, desc) \
struct name : event::traits_base<this_type, type, this_type::id_>         \
{                                                               \
    EMBR_INTERNAL_PROPERTY_TRAITS_BODY(this_type, type, this_type::id_, desc) \
};  \
EMBR_PROPERTY_ID_LOOKUP(name, this_type::id_);

#define EMBR_PROPERTY_ID2_BASE(name, type, id_, desc) \
type name##_;                                    \
struct name : event::traits_base<this_type, type, id_> \
{                                                  \
    EMBR_INTERNAL_PROPERTY_TRAITS_BODY(this_type, type, id_, desc); \
    EMBR_PROPERTY_TRAITS_GETTER_SETTER(struct id, name##_) \
    static inline struct id& host(this_type& o) { return o.fields_; } \
}

#define EMBR_PROPERTY_ID2_2(name, type, id_, desc) \
EMBR_PROPERTY_ID2_BASE(name, type, id_, desc);     \
EMBR_PROPERTY_ID_LOOKUP(name, id_)

#define EMBR_PROPERTY_ID2_1(name, type, desc)   EMBR_PROPERTY_ID2_BASE(name, type, -2, desc);

// Guidance from
// https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME
#define EMBR_PROPERTY_ID2(...) GET_MACRO(__VA_ARGS__, EMBR_PROPERTY_ID2_2, EMBR_PROPERTY_ID2_1)(__VA_ARGS__)
#define EMBR_PROPERTY_ID_EXT(...) GET_MACRO(__VA_ARGS__, EMBR_PROPERTY_ID_ALIAS, EMBR_PROPERTY_ID)(__VA_ARGS__)


#define EMBR_PROPERTY_BEGIN \
struct id : event::lookup_tag  \
{\
    template <int id_, bool = true> struct lookup;

#define EMBR_PROPERTY_END };
#define EMBR_PROPERTY_END2 } fields_;




namespace embr { namespace experimental {


// Through lookup mechanism, resolves TOwner + property ID back to property traits
template <class TOwner, int id_>
using PropertyTraits3 = typename TOwner::id::template lookup<id_>;


namespace event {

// TODO: Consider changing this to property_traits_tag, although
// if under 'properties' namespace perhaps not needed
struct traits_tag {};

// TODO: Consider changing this to properties_tag
struct lookup_tag {};

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

template <typename T = void, int id = -1, class enabled = void>
struct PropertyChanged;


template <typename T, int id = -1, class enabled = void>
struct PropertyChanging;


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

template <class TSubject = embr::void_subject>
class PropertyNotifier : public TSubject
{
    typedef TSubject subject_type;

protected:
    template <typename TTrait, class TContext>
    void fire_changed3(typename TTrait::value_type v, TContext& context)
    {
        typename TTrait::owner_type& owner = context;
        subject_type::notify(event::PropertyChanged<TTrait>(&owner, v), context);
    }

    template <int id_, class TOwner, class T, class TImpl>
    void fire_changed4(T v, TImpl& context)
    {
        TOwner& owner = context;    // Give conversions a chance to run

        subject_type::notify(event::PropertyChanged<TOwner, id_>{&owner, v}, context);
    }

    template <typename TTrait, class TContext>
    void fire_changing(
        const typename TTrait::value_type& v_old,
        const typename TTrait::value_type& v, TContext& context)
    {
        typename TTrait::owner_type& owner = context;    // Give conversions a chance to run

        subject_type::notify(event::PropertyChanging<TTrait>(owner, v_old, v), context);
    }

    template <int id, class TOwner, class T, class TContext>
    void fire_changing(
        const T& v_old,
        const T& v, TContext& context)
    {
        TOwner& owner = context;    // Give conversions a chance to run

        subject_type::notify(event::PropertyChanging<TOwner, id>{&owner, v_old, v}, context);
    }

    template <class TTraits, class TContext>
    void setter(typename TTraits::value_type v, TContext& context)
    {
        typedef TTraits traits_type;
        constexpr int id = traits_type::id();
        typedef typename TTraits::owner_type owner_type;
        auto& host = TTraits::host(context);
        owner_type& impl = context;    // give conversion a chance
//#ifdef DEBUG
        const char* name = traits_type::name();
        const char* owner_name = owner_type::name();
//#endif
        auto current_v = traits_type::get(host);

        if(current_v != v)
        {
            fire_changing<traits_type>(current_v, v, context);
            traits_type::set(host, v);
            fire_changed3<traits_type>(v, context);
        }
    }

    template <int id, class TOwner, typename T, class TImpl>
    inline void setter(T v, TImpl& impl)
    {
        setter<PropertyTraits3<TOwner, id> >(v, impl);
    }

public:
    PropertyNotifier() = default;

    PropertyNotifier(const TSubject& subject) : subject_type(subject)
    {}

    PropertyNotifier(TSubject&& subject) : subject_type(std::move(subject))
    {}
};

template <class TImpl, class TSubject>
class PropertyHost : public PropertyNotifier<TSubject>,
    public TImpl
{
protected:
    typedef PropertyNotifier<TSubject> base_type;
    //using subject_base = typename base_type::subject_type;

public:
    typedef TImpl impl_type;
    //impl_type impl_;

protected:
    impl_type& impl() { return *this; }
    const impl_type& impl() const { return *this; }

    template <int id, typename T>
    void setter(T v)
    {
        //impl_type& context = impl();
        TSubject& subject = *this;
        typename impl_type::template responder<TSubject, impl_type> context{impl(), subject};
        base_type::template setter<id, impl_type>(v, context);
    }

    template <typename TTraits>
    void setter(typename TTraits::value_type v)
    {
        typedef typename TTraits::owner_type owner_type;
        TSubject& subject = *this;
        typename impl_type::template responder<TSubject, impl_type> context{impl(), subject};
        base_type::template setter<TTraits>(v, context);
    }

public:
    PropertyHost() = default;

    PropertyHost(const TSubject& subject) :
        base_type(subject) {}

    PropertyHost(const TImpl& impl, const TSubject& subject) :
        base_type(subject),
        TImpl(impl) {}

    PropertyHost(TSubject&& subject) : base_type(std::move(subject)) {}
};


}}