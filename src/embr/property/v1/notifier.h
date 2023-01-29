#pragma once

#include "event.h"
#include "macros.h"
#include "traits.h"

#include "../../exp/rtti.h"

namespace embr {

inline namespace property { inline namespace v1 {

struct PropertyContainer
{
    typedef PropertyContainer this_type;

    constexpr static const char* name() { return "Generic property host"; }

    // DEBT: Was going to call this host, but macros refer to the impl portion as host
    // so work out naming - in the meantime, we'll call this runtime
    template <class TSubject, class TImpl>
    using runtime = embr::property::v1::PropertyHost<TImpl, TSubject>;

    // Helper alias to wrap up host/runtime into a reference-only flavor
    template <class TSubject, class TImpl>
    using context = typename TImpl::template runtime<
        estd::reference_wrapper<TSubject>, estd::reference_wrapper<TImpl> >;
};

// Guidance from https://stackoverflow.com/questions/59984423/stdreference-wrapper-unwrap-the-wrapper
// DEBT: Consider putting this into estd, since it's such a companion to reference_wrapper
namespace detail {

template <typename type_t, class  orig_t>
struct unwrap_impl
{
    using type = orig_t;
    using is_wrapped = estd::false_type;
};

template <typename type_t, class V>
struct unwrap_impl<estd::reference_wrapper<type_t>,V>
{
    using type = type_t;
    using is_wrapped = estd::true_type;
};
}

template<class T>
struct unwrap
{
protected:
    typedef typename detail::unwrap_impl<estd::decay_t<T>, T> impl;

public:
    using type = typename impl::type;
    using is_wrapped = typename impl::is_wrapped;
};

template <typename type_t>
using unwrap_t = typename unwrap<type_t>::type;


template <class TSubject = embr::void_subject>
class PropertyNotifier : public TSubject
{
    typedef TSubject subject_type;

protected:
    subject_type& subject() { return *this; }

    using is_stateless_subject = estd::is_base_of<embr::internal::tag::stateless_subject, subject_type>;

    template <typename TTrait, class TContext>
    void fire_changed(typename TTrait::value_type v, TContext& context)
    {
        typename TTrait::owner_type& owner = context;
        event::PropertyChanged<TTrait> e(&owner, v);
        subject_type::notify(e, context);
    }

    template <class TOwner, int id_, class T, class TImpl>
    void fire_changed(T v, TImpl& context)
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
        //constexpr int id = traits_type::id();
        auto& store = TTraits::store(context);
        //owner_type& impl = context;    // give conversion a chance
#ifdef UNIT_TESTING
        typedef typename TTraits::owner_type owner_type;
        const char* name = traits_type::name();
        const char* owner_name = owner_type::name();
#endif
        auto current_v = traits_type::get(store);

        if(current_v != v)
        {
            fire_changing<traits_type>(current_v, v, context);
            traits_type::set(store, v);
            fire_changed<traits_type>(v, context);
        }
    }

    template <int id, class TOwner, typename T, class TImpl>
    inline void setter(T v, TImpl& impl)
    {
        setter<PropertyTraits<TOwner, id> >(v, impl);
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
    using base_type::subject;
    //using subject_base = typename base_type::subject_type;

public:
    typedef unwrap_t<TImpl> impl_type;
    //typedef TImpl impl_type;
    //impl_type impl_;
    using context_type = typename impl_type::template context<TSubject, impl_type>;
    using runtime_type = typename impl_type::template runtime<TSubject, impl_type>;

    // If we don't do this, we get caught in an endless recursion loop doing type conversion
    // from reference_wrapper to impl_type
    // DEBT: Very brute force bringing in a temporary to do our dirty work.  Specializing on
    // reference wrapper didn't work though.
    static constexpr impl_type& unwrap_assist(TImpl* _this)
    {
        return estd::reference_wrapper<impl_type>(*_this).get();
    }

    static constexpr const impl_type& unwrap_assist(const TImpl* _this)
    {
        return estd::reference_wrapper<const impl_type>(*_this).get();
    }

    // DEBT: Temporarily making this public because operator() casting goes crazy, and
    // reference_wrapper doesn't seem to be working nice with deduction/specialization
    impl_type& impl() { return unwrap_assist(this); }
    const impl_type& impl() const { return unwrap_assist(this); }

    inline operator impl_type& ()
    {
        return impl();
    }

    inline operator const impl_type& () const { return impl(); }

    // DEBT: This is scary stuff - penalty for failure is high, i.e.
    // memory corruption.  That's why mostly we use context
    runtime_type* runtime()
    {
        // Meager protection against using the 'context' variety.  Still dangerous
        static_assert(unwrap<TImpl>::is_wrapped::value == false,
            "Must operate on direct impl, not a wrapped reference");

        return (runtime_type*) this;
    }

protected:

    // Helpers for those who really want to fire these events themselves

    template <class TEvent>
    void notify(TEvent&& e)
    {
        // NOTE: Don't worry, CLion/GDB reports somewhat misleading types here but we really do
        // seem to have impl_type giving us real 'runtime'
        context_type context{impl(), subject()};
        base_type::notify(std::move(e), context);
    }

    template <class TTraits>
    inline void fire_changed(typename TTraits::value_type v)
    {
        context_type context{impl(), subject()};
        base_type::template fire_changed<TTraits>(v, context);
    }

    template <class TOwner, int id, class T>
    inline void fire_changed(T v)
    {
        context_type context{impl(), subject()};
        base_type::template fire_changed<TOwner, id>(v, context);
    }

protected:

    template <int id, typename T>
    void setter(T v)
    {
        context_type context{impl(), subject()};
        base_type::template setter<id, impl_type>(v, context);
    }

    template <class TTraits>
    constexpr typename TTraits::value_type getter() const
    {
        typedef TTraits traits_type;
        //typedef typename TTraits::owner_type owner_type;
        return traits_type::get(traits_type::store(impl()));
    }

    template <typename TTraits>
    void setter(typename TTraits::value_type v)
    {
        context_type context{impl(), subject()};
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

#if UNIT_TESTING
    runtime_type* debug_runtime() { return runtime(); }
#endif
};

}}

}