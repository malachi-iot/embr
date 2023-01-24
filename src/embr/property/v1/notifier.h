#pragma once

#include "event.h"
#include "macros.h"
#include "traits.h"

namespace embr {

inline namespace property { inline namespace v1 {

namespace impl {

struct PropertyHost
{
    typedef PropertyHost this_type;

    constexpr static const char* name() { return "Generic property host"; }

    // DEBT: Was going to call this host, but macros refer to the impl portion as host
    // so work out naming - in the meantime, we'll call this runtime
    template <class TSubject, class TImpl>
    using runtime = embr::property::v1::PropertyHost<TImpl, TSubject>;

    template <class TSubject, class TImpl>
    using context = typename TImpl::template runtime<
        estd::reference_wrapper<TSubject>, estd::reference_wrapper<TImpl> >;
};

}

// Guidance from https://stackoverflow.com/questions/59984423/stdreference-wrapper-unwrap-the-wrapper
namespace detail {

template <typename type_t, class  orig_t>
struct unwrap_impl
{
    using type = orig_t;
};

template <typename type_t, class V>
struct unwrap_impl<estd::reference_wrapper<type_t>,V>
{
    using type = type_t;
};
}

template<class T>
struct unwrap
{
    using type = typename detail::unwrap_impl<estd::decay_t<T>, T>::type;
};

template <typename type_t>
using unwrap_t = typename unwrap<type_t>::type;


template <class TSubject = embr::void_subject>
class PropertyNotifier : public TSubject
{
    typedef TSubject subject_type;

protected:
    subject_type& subject() { return *this; }

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
        auto& store = TTraits::store(context);
        owner_type& impl = context;    // give conversion a chance
//#ifdef DEBUG
        const char* name = traits_type::name();
        const char* owner_name = owner_type::name();
//#endif
        auto current_v = traits_type::get(store);

        if(current_v != v)
        {
            fire_changing<traits_type>(current_v, v, context);
            traits_type::set(store, v);
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
    typedef unwrap_t<TImpl> impl_type;
    //typedef TImpl impl_type;
    //impl_type impl_;

    // DEBT: Temporarily making this public because operator() casting goes crazy, and
    // reference_wrapper doesn't seem to be working nice with deduction/specialization
    impl_type& impl() { return *this; }
    const impl_type& impl() const { return *this; }

    // Doesn't seem to help above use case
    /*
    inline operator impl_type& ()
    {
        // Need to do this explicit trickery rather than return *this
        // otherwise we get in a recursive loop
        TImpl& i = *this;
        return i;
    }   */

protected:

    template <int id, typename T>
    void setter(T v)
    {
        //impl_type& context = impl();
        TSubject& subject = *this;
        typename impl_type::template context<TSubject, impl_type> context{impl(), subject};
        base_type::template setter<id, impl_type>(v, context);
    }

    template <class TTraits>
    constexpr typename TTraits::value_type getter() const
    {
        typedef TTraits traits_type;
        typedef typename TTraits::owner_type owner_type;
        return traits_type::get(traits_type::store(impl()));
    }

    template <typename TTraits>
    void setter(typename TTraits::value_type v)
    {
        typedef typename TTraits::owner_type owner_type;
        TSubject& subject = *this;
        typename impl_type::template context<TSubject, impl_type> context{impl(), subject};
        base_type::template setter<TTraits>(v, context);
    }

public:
    PropertyHost() = default;

    PropertyHost(TSubject& subject) :
        base_type(subject) {}

    PropertyHost(const TSubject& subject) :
        base_type(subject) {}

    PropertyHost(const TImpl& impl, const TSubject& subject) :
        base_type(subject),
        TImpl(impl) {}

    PropertyHost(TSubject&& subject) : base_type(std::move(subject)) {}

    //operator impl_type& () { return *this; }
};

}}

}