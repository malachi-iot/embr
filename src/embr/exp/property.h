#pragma once

#include "../observer.h"

#define EMBR_PROPERTY_TRAITS_BODY(owner, type, id, desc) \
typedef event::traits_base<owner, type, id> base_type; \
typedef owner owner_type;                              \
using typename base_type::value_type;                  \
static constexpr const char* name() { return desc; }

#define EMBR_PROPERTY_TRAITS_SPARSE_BASE(owner, type, id, desc) \
    event::traits_base<owner, type, id>         \
{                                                               \
    EMBR_PROPERTY_TRAITS_BODY(owner, type, id, desc) \
}


#define EMBR_PROPERTY_TRAITS_BASE(owner, name_, id, desc) \
    event::traits_base<owner, decltype(owner::name_), id>         \
{                                                          \
    EMBR_PROPERTY_TRAITS_BODY(owner, decltype(owner::name_), id, desc) \
    static constexpr value_type get(owner_type& o)       \
    { return o.name_; }                                    \
    static inline void set(owner_type& o, value_type v)       \
    { o.name_ = v; }                                       \
}

#define EMBR_PROPERTY_ID_ALIAS(name, id_, alias, desc) \
    struct alias : EMBR_PROPERTY_TRAITS_BASE(this_type, name, this_type::id_, desc);    \
    template <bool dummy> struct lookup<id_, dummy> : alias {};

#define EMBR_PROPERTY_ID(name, id, desc) EMBR_PROPERTY_ID_ALIAS(name, id, name, desc)
#define EMBR_PROPERTY_SPARSE_ID(name, type, id_, desc)   \
    struct name : EMBR_PROPERTY_TRAITS_SPARSE_BASE(this_type, type, id_, desc); \
    template <bool dummy> struct lookup<id_, dummy> : name {};

#define EMBR_PROPERTY_BEGIN \
struct id : event::lookup_tag  \
{\
    template <int id_, bool = true> struct lookup;

#define EMBR_PROPERTY_END };




namespace embr { namespace experimental {


template <class TOwner, int id_>
struct PropertyTraits3 : TOwner::id::template lookup<id_> {};


namespace event {

struct traits_tag {};

// Phasing this one out in favor of id::lookup_tag presence
struct owner_tag {};

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
};

template <typename T = void, int id = -1, class enabled = void>
struct PropertyChanged;


template <typename T, int id = -1, class enabled = void>
struct PropertyChanging;


namespace internal {

template <class TOwner, class TValue, class enabled = void>
struct PropertyChanged
{
    typedef TOwner owner_type;
    typedef TValue value_type;

    owner_type* const owner;
    const value_type value;
};

template <class TOwner>
struct PropertyChanged<TOwner, void>
{
    typedef TOwner owner_type;

    owner_type* const owner;
};

// DEBT: Fix naming and make this a specialization of PropertyChanged if we can
// FIX: Not using anyway because initializer list doesn't flow through to base class smoothly
template <class TTraits>
struct PropertyChangedTraits :
    PropertyChanged<typename TTraits::owner_type, typename TTraits::value_type>
{};

}


// 100% runtime flavor - that being the case, we can't practically provide 'value' itself
template <>
struct PropertyChanged<>
{
private:
public:
};

template <typename TEnum>
struct PropertyChanged<TEnum, -1, typename estd::enable_if<
    //!estd::is_base_of<traits_tag, T>::value &&
    //!estd::is_base_of<owner_tag, T>::value &&
    estd::is_enum<TEnum>::value
>::type>
{
    typedef TEnum value_type;

    const int id_;
    int id() const { return id_; }

    // DEBT: This field not passing tests yet, but haven't looked into it much
    void* owner;

    const value_type value;

    PropertyChanged(int id, value_type value) :
        id_{id},
        value{value}
    {}

    template <int id_>
    PropertyChanged(const PropertyChanged<value_type, id_>& copy_from) :
        owner(copy_from.owner),
        id_{copy_from.id()},
        value{copy_from.value}
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

    // For converting traits flavor to baseline (this one) flavor
    template <class T2,
        typename estd::enable_if<
            estd::is_base_of<traits_tag, T2>::value &&
            estd::is_same<typename T2::value_type, value_type>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<T2, -1>& copy_from) :
        owner(copy_from.owner),
        id_{copy_from.id()},
        value(copy_from.value)
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

template <typename TTraits>
struct PropertyChanged<TTraits, -1, typename estd::enable_if<
    estd::is_base_of<traits_tag, TTraits>::value
>::type> : internal::PropertyChanged<typename TTraits::owner_type, typename TTraits::value_type>
{
    typedef internal::PropertyChanged<
        typename TTraits::owner_type, typename TTraits::value_type> base_type;

    using typename base_type::value_type;
    using typename base_type::owner_type;

    static constexpr int id() { return TTraits::id(); }
    static const char* name() { return TTraits::name(); }

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
    //estd::is_base_of<owner_tag, TOwner>::value ||
    estd::is_base_of<typename TOwner::id::lookup_tag, typename TOwner::id>::value
>::type> : PropertyTraits3<TOwner, id_>
{
    typedef PropertyTraits3<TOwner, id_> traits;
    typedef typename traits::value_type value_type;

    TOwner* const owner;
    const value_type value;

    PropertyChanged(TOwner* owner, value_type v) : owner(owner), value(v) {}

    // For converting traits flavor to owner (this one) flavor
    template <class T2,
        typename estd::enable_if<
            estd::is_base_of<traits_tag, T2>::value &&
            estd::is_same<typename T2::owner_type, TOwner>::value &&
            estd::is_same<typename T2::value_type, value_type>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<T2, -1>& copy_from) :
        owner(copy_from.owner),
        value(copy_from.value)
    {}
};


template <typename TOwner>
struct PropertyChanged<TOwner, -1, typename estd::enable_if<
    //estd::is_base_of<owner_tag, TOwner>::value
    estd::is_base_of<typename TOwner::id::lookup_tag, typename TOwner::id>::value
>::type>
{
    TOwner* const owner;
    const int id_;

    int id() const { return id_; }

    PropertyChanged(TOwner* owner, int v) : owner(owner), id_(v) {}

    template <int id>
    PropertyChanged(const PropertyChanged<TOwner, id>& copy_from) :
        owner(copy_from.owner),
        id_(copy_from.id())
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
        owner_type& impl = context;    // give conversion a chance
//#ifdef DEBUG
        const char* name = traits_type::name();
        const char* owner_name = owner_type::name();
//#endif
        auto current_v = traits_type::get(impl);

        if(current_v != v)
        {
            fire_changing<traits_type>(current_v, v, context);
            traits_type::set(impl, v);
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