#pragma once

#include "../observer.h"

#define EMBR_PROPERTY_TRAITS_BASE(owner, name_, id, desc) \
    event::traits_base<decltype(owner::name_), id>         \
{                                                          \
    typedef event::traits_base<decltype(owner::name_), id> base_type; \
    typedef owner owner_type;                              \
    using typename base_type::value_type;                  \
    static constexpr const char* name() { return desc; }   \
    static constexpr value_type get(owner_type& o)       \
    { return o.name_; }                                    \
    static inline void set(owner_type& o, value_type v)       \
    { o.name_ = v; }                                       \
}

#define EMBR_PROPERTY_ID2(name, id, desc) \
    struct name : EMBR_PROPERTY_TRAITS_BASE(this_type, name, this_type::id, desc);

#define EMBR_PROPERTY_DECLARATION2(owner, name_)  \
template <> \
struct PropertyTraits2<owner, owner::id::name_::id()> : \
    owner::id::name_ {};



namespace embr { namespace experimental {


template <class T, T id_>
struct PropertyTraits;

template <class T, int id_>
struct PropertyTraits2;

template <class TOwner, int id_>
struct PropertyTraits3 : TOwner::id::template lookup<id_> {};


namespace event {

struct traits_tag {};

struct owner_tag {};

struct lookup_tag {};

template <class T, int id_>
struct traits_base : traits_tag
{
    typedef T value_type;
    static constexpr int id() { return id_; }
};

template <typename T, int id = -1, class enabled = void>
struct PropertyChanged;


template <typename T, int id = -1, class enabled = void>
struct PropertyChanging;


template <typename T>
struct PropertyChanged<T, -1, typename estd::enable_if<
    !estd::is_base_of<traits_tag, T>::value &&
    !estd::is_base_of<owner_tag, T>::value
>::type>
{
    const int id_;
    int id() const { return id_; }

    const T value;

    PropertyChanged(int id, T value) :
        id_{id},
        value{value}
    {}

    template <int id_>
    PropertyChanged(const PropertyChanged<T, id_>& copy_from) :
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
            estd::is_same<typename T2::value_type, T>::value
            , bool>::type = true>
    PropertyChanged(const PropertyChanged<T2, -1>& copy_from) :
        id_{copy_from.id()},
        value(copy_from.value)
    {}
};

template <typename T, int id_>
struct PropertyChanged<T, id_, typename estd::enable_if<
    !estd::is_base_of<traits_tag, T>::value &&
    !estd::is_base_of<owner_tag, T>::value &&
    id_ >= 0>::type>
{
    const T value;

    static constexpr int id() { return id_; }

    PropertyChanged(T value) : value{value} {}
};

template <typename T>
struct PropertyChanged<T, -1, typename estd::enable_if<
    estd::is_base_of<traits_tag, T>::value
>::type>
{
    typedef typename T::value_type value_type;

    const value_type value;

    static constexpr int id() { return T::id(); }
    static const char* name() { return T::name(); }

    PropertyChanged(value_type value) : value{value} {}
};


template <typename TTraits>
struct PropertyChanging<TTraits, -1, typename estd::enable_if<
    estd::is_base_of<traits_tag, TTraits>::value
>::type> :
    TTraits
{
    typedef typename TTraits::value_type value_type;

    const value_type old_value;
    const value_type new_value;

    PropertyChanging(value_type old, value_type new_value) :
        old_value{old},
        new_value{new_value}
    {}
};


template <typename TOwner, int id_>
struct PropertyChanged<TOwner, id_, typename estd::enable_if<
    estd::is_base_of<owner_tag, TOwner>::value
>::type> : PropertyTraits2<TOwner, id_>
{
    typedef PropertyTraits2<TOwner, id_> traits;
    typedef typename traits::value_type value_type;

    TOwner* owner;
    const value_type value;

    PropertyChanged(TOwner* owner, value_type v) : owner(owner), value(v) {}
};


template <typename TOwner>
struct PropertyChanged<TOwner, -1, typename estd::enable_if<
    estd::is_base_of<owner_tag, TOwner>::value
>::type>
{
    TOwner* owner;
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
    estd::is_base_of<owner_tag, TOwner>::value
>::type> : PropertyTraits2<TOwner, id_>
{
    typedef PropertyTraits2<TOwner, id_> traits_type;
    typedef typename traits_type::value_type value_type;

    TOwner* owner;
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
    template <typename T, class TContext>
    void fire_changed(int id, T v, TContext& context)
    {
        subject_type::notify(event::PropertyChanged<T>{id, v}, context);
    }

    template <int id, typename T, class TContext>
    void fire_changed2(T v, TContext& context)
    {
        subject_type::notify(event::PropertyChanged<T, id>{v}, context);
    }

    template <typename TTrait, class TContext>
    void fire_changed3(typename TTrait::value_type v, TContext& context)
    {
        subject_type::notify(event::PropertyChanged<TTrait>{v}, context);
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
        subject_type::notify(event::PropertyChanging<TTrait>{v_old, v}, context);
    }

    template <int id, class TOwner, class T, class TContext>
    void fire_changing4(
        const T& v_old,
        const T& v, TContext& context)
    {
        TOwner& owner = context;    // Give conversions a chance to run

        subject_type::notify(event::PropertyChanging<TOwner, id>{&owner, v_old, v}, context);
    }

    template <int id, class TImpl>
    static typename PropertyTraits2<TImpl, id>::value_type getter(TImpl& impl)
    {
        typedef PropertyTraits2<TImpl, id> traits_type;

        return traits_type::get(impl);
    }

    template <class TTraits, class TImpl>
    void setter(typename TTraits::value_type v, TImpl& impl)
    {
        typedef TTraits traits_type;
        constexpr int id = traits_type::id();
        typedef typename TTraits::owner_type owner_type;
//#ifdef DEBUG
        const char* name = traits_type::name();
//#endif
        auto current_v = traits_type::get(impl);

        if(current_v != v)
        {
            fire_changing4<id, owner_type>(current_v, v, impl);
            traits_type::set(impl, v);
            fire_changed4<id, owner_type>(v, impl);
        }
    }

    template <int id, class TOwner, typename T, class TImpl>
    inline void setter(T v, TImpl& impl)
    {
        setter<PropertyTraits2<TOwner, id> >(v, impl);
    }


    template <int id, class TOwner, typename T, class TImpl>
    void setter3(T v, TImpl& impl)
    {
        typedef PropertyTraits3<TOwner, id> traits_type;

//#ifdef DEBUG
        const char* name = traits_type::name();
//#endif
        T current_v = traits_type::get(impl);

        if(current_v != v)
        {
            fire_changing4<id, TOwner>(current_v, v, impl);
            traits_type::set(impl, v);
            fire_changed4<id, TOwner>(v, impl);
        }
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

    template <int id, typename T>
    void setter(T v)
    {
        //impl_type& context = impl();
        TSubject& subject = *this;
        typename impl_type::template responder<TSubject> context{impl(), subject};
        base_type::template setter<id, impl_type>(v, context);
    }

public:
    PropertyHost() = default;
    PropertyHost(const TImpl& impl, const TSubject& subject) :
        base_type(subject),
        TImpl(impl) {}

    PropertyHost(TSubject&& subject) : base_type(std::move(subject)) {}
};


}}