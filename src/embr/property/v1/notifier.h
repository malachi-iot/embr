#pragma once

#include "event.h"
#include "macros.h"
#include "traits.h"
#include "unwrap.h"

#include "../../exp/rtti.h"

namespace embr {

inline namespace property { inline namespace v1 {

// Safer but more limited way to sling subject/runtime around when enabled
// If we could completely eliminate runtime_type downcast concerns, we could
// subsequently eliminate this feature
#ifndef FEATURE_EMBR_PROPERTY_CONTEXT
#define FEATURE_EMBR_PROPERTY_CONTEXT 0
#endif

struct PropertyContainer
{
private:
    typedef PropertyContainer this_type;

public:
    constexpr static const char* name() { return "Generic property container"; }

    // DEBT: Was going to call this host, but macros refer to the impl portion as host
    // so work out naming - in the meantime, we'll call this runtime.  Calling it
    // runtime has a minor downside, it can be confusing to call it runtime when
    // compile-time evaluations take place, such as with verify_runtime_integrity
    template <class TSubject, class TImpl>
    using runtime = embr::property::v1::PropertyHost<TImpl, TSubject>;

#if FEATURE_EMBR_PROPERTY_CONTEXT
    // Helper alias to wrap up host/runtime into a reference-only flavor
    template <class TSubject, class TImpl>
    using context = typename TImpl::template runtime<
        estd::reference_wrapper<TSubject>, estd::reference_wrapper<TImpl> >;
#endif

    // NOTE: This is the best candidate so far for static_factory behavior.  Needs more integration
    // work, but doesn't have incomplete type issues
    template <class TSubject, class TService>
    struct static_factory
    {
        typedef embr::unwrap_t<TSubject> subject_type;
        typedef typename TService::template runtime<subject_type> runtime_type;

        static runtime_type instance;

        using static_type = estd::integral_constant<runtime_type*, &instance>;
    };

protected:
    template <class T>
    using changing = event::PropertyChanging<T>;

    template <class T>
    using changed = event::PropertyChanged<T>;
};

template <class TSubject, class TService>
typename embr::PropertyContainer::static_factory<TSubject, TService>::runtime_type
        embr::PropertyContainer::static_factory<TSubject, TService>::instance;


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

        // Quiet down warnings
        (void)name;
        (void)owner_name;
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

    constexpr explicit PropertyNotifier(const TSubject& subject) :
        subject_type(subject)
    {}

    constexpr explicit PropertyNotifier(TSubject&& subject) :
        subject_type(std::forward<TSubject>(subject))
    {}
};

// DEBT: Once naming conventions settle down, split this out of notifier.h
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
#if FEATURE_EMBR_PROPERTY_CONTEXT
    using context_type = typename impl_type::template context<TSubject, impl_type>;
#endif
    using runtime_type = typename impl_type::template runtime<TSubject, impl_type>;

    // EXPERIMENTAL
    // likely gonna get incomplete type here
    struct static_factory_exp
    {
        typedef embr::unwrap_t<TSubject> subject_type;
        // DEBT: Confusing, this runtime_type differs from parent scope
        typedef typename impl_type::template runtime<subject_type, impl_type> runtime_type;

        static runtime_type instance;
    };

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

    // EXPERIMENTAL
    // This ensures that at least the 'tbis' in question is the base type of
    // runtime_type.  That goes a very long way, because both state their
    // TSubject and TImpl, which constitutes all the state allowed to a runtime.
    // Caveat is that generally 'T' is the root Service::runtime<> - meaning
    // it's conceivable a "bad runtime" with the right impl & subject could come
    // through here.  In other words -
    // Happy Path: Service::runtime<> -> Test1::runtime<TSubject, Test1>
    // Sad path: Service::runtime<> -> Test2::runtime<TSubject, Test1>
    // However, as long as runtimes don't maintain state - the deviation in behaviors
    // may not be of consequence, since the correct impl/subject is in play
    template <class T>
    static constexpr bool verify_runtime_integrity(const T*)
    {
        static_assert(estd::is_base_of<T, runtime_type>::value,
                "runtime_type does not match 'this' type");
        return true;
    }

protected:

    // Helpers for those who really want to fire these events themselves

    template <class TEvent>
    void notify(TEvent&& e)
    {
#if FEATURE_EMBR_PROPERTY_CONTEXT
        // NOTE: Don't worry, CLion/GDB reports somewhat misleading types here but we really do
        // seem to have impl_type giving us real 'runtime'
        context_type context{impl(), subject()};
#else
        runtime_type& context = *runtime();
#endif
        base_type::notify(std::move(e), context);
    }

    template <class TTraits>
    inline void fire_changing(typename TTraits::value_type v_old,
        typename TTraits::value_type v)
    {
#if FEATURE_EMBR_PROPERTY_CONTEXT
        context_type context{impl(), subject()};
#else
        runtime_type& context = *runtime();
#endif

        base_type::template fire_changing<TTraits>(v_old, v, context);
    }

    template <class TTraits>
    inline void fire_changed(typename TTraits::value_type v)
    {
#if FEATURE_EMBR_PROPERTY_CONTEXT
        context_type context{impl(), subject()};
#else
        runtime_type& context = *runtime();
#endif
        base_type::template fire_changed<TTraits>(v, context);
    }

    template <class TOwner, int id, class T>
    inline void fire_changed(T v)
    {
#if FEATURE_EMBR_PROPERTY_CONTEXT
        context_type context{impl(), subject()};
#else
        runtime_type& context = *runtime();
#endif
        base_type::template fire_changed<TOwner, id>(v, context);
    }

    template <typename TTrait>
    void fire_changed_null_owner(typename TTrait::value_type v)
    {
#if FEATURE_EMBR_PROPERTY_CONTEXT
        context_type context{impl(), subject()};
#else
        runtime_type& context = *runtime();
#endif
        event::PropertyChanged<TTrait> e(nullptr, v);
        subject().notify(e, context);
    }

protected:

    template <int id, typename T>
    void setter(T v)
    {
#if FEATURE_EMBR_PROPERTY_CONTEXT
        context_type context{impl(), subject()};
#else
        runtime_type& context = *runtime();
#endif
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
#if FEATURE_EMBR_PROPERTY_CONTEXT
        context_type context{impl(), subject()};
#else
        runtime_type& context = *runtime();
#endif
        base_type::template setter<TTraits>(v, context);
    }

public:
    template <class ...TArgs, class TImpl2 = TImpl,
        estd::enable_if_t<unwrap<TImpl2>::is_wrapped::value == false, bool> = true >
    constexpr explicit PropertyHost(TArgs&&...args) :
        impl_type(std::forward<TArgs>(args)...)
    {}

    constexpr explicit PropertyHost(TSubject& subject) :
        base_type(subject) {}

    constexpr explicit PropertyHost(const TSubject& subject) :
        base_type(subject) {}

    PropertyHost(const TImpl& impl, const TSubject& subject) :
        base_type(subject),
        TImpl(impl) {}

    constexpr explicit PropertyHost(TSubject&& subject) :
        base_type(std::forward<TSubject>(subject)) {}

    //operator impl_type& () { return *this; }

#if UNIT_TESTING
    runtime_type* debug_runtime() { return runtime(); }
#endif
};

}}

}
