#pragma once

#define EMBR_INTERNAL_PROPERTY_TRAITS_BODY(owner, type, id, desc) \
typedef embr::internal::property::traits_base<owner, type, id> base_type; \
static constexpr const char* name() { return desc; }


#define EMBR_PROPERTY_TRAITS_GETTER_SETTER(storage_type, name_) \
    using typename base_type::value_type;                  \
    static constexpr value_type get(const storage_type& o)       \
    { return o.name_; }                                   \
    static inline void set(storage_type& o, value_type v)   \
    { o.name_ = v; }

#define EMBR_PROPERTY_ID_LOOKUP(name, id_)  \
template <bool dummy> struct lookup<id_, dummy> : name {}

#define EMBR_PROPERTY_ID_ALIAS(name_, id_, alias, desc) \
struct alias : embr::internal::property::traits_base<this_type, decltype(this_type::name_), id_>         \
{                                                          \
    EMBR_INTERNAL_PROPERTY_TRAITS_BODY(this_type, decltype(this_type::name_), id_, desc) \
    EMBR_PROPERTY_TRAITS_GETTER_SETTER(owner_type, name_)  \
    static constexpr owner_type& store(owner_type& o) { return o; } \
    static constexpr const owner_type& store(const owner_type& o) { return o; } \
};  \
EMBR_PROPERTY_ID_LOOKUP(alias, id_);

#define EMBR_INTERNAL_PROPERTY_ID_EXT(name, id, desc) EMBR_PROPERTY_ID_ALIAS(name, this_type::id, name, desc)
#define EMBR_INTERNAL_PROPERTY_ID_SPARSE(name, type, id_, desc) \
struct name : traits<type, this_type::id_>         \
{                                                               \
    EMBR_INTERNAL_PROPERTY_TRAITS_BODY(this_type, type, this_type::id_, desc) \
};  \
EMBR_PROPERTY_ID_LOOKUP(name, this_type::id_);

#define EMBR_INTERNAL_PROPERTY_ID_SPARSE2(name_, type, desc) \
struct name_ : traits<type>         \
{                                                               \
    static constexpr const char* name() { return desc; } \
};


#define EMBR_PROPERTY_ID2_BASE(name, type, id_, desc) \
type name##_ = type();  \
struct name : embr::internal::property::traits_base<this_type, type, id_> \
{                                                  \
    EMBR_INTERNAL_PROPERTY_TRAITS_BODY(this_type, type, id_, desc); \
    EMBR_PROPERTY_TRAITS_GETTER_SETTER(struct id, name##_) \
    static constexpr struct id& store(this_type& o) { return o.fields_; } \
    static constexpr const struct id& store(const this_type& o) { return o.fields_; } \
}

#define EMBR_PROPERTY_ID2_2(name, type, id_, desc) \
EMBR_PROPERTY_ID2_BASE(name, type, id_, desc);     \
EMBR_PROPERTY_ID_LOOKUP(name, id_)

#define EMBR_PROPERTY_ID2_1(name, type, desc)   EMBR_PROPERTY_ID2_BASE(name, type, -2, desc);

// Guidance from
// https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
#define GET_MACRO(_1,_2,_3,_4,NAME,...) NAME
#define EMBR_PROPERTY_ID(...) GET_MACRO(__VA_ARGS__, EMBR_PROPERTY_ID2_2, EMBR_PROPERTY_ID2_1)(__VA_ARGS__)
#define EMBR_PROPERTY_ID_EXT(...) GET_MACRO(__VA_ARGS__, EMBR_PROPERTY_ID_ALIAS, EMBR_INTERNAL_PROPERTY_ID_EXT)(__VA_ARGS__)
#define EMBR_PROPERTY_ID_SPARSE(...) GET_MACRO(__VA_ARGS__, EMBR_INTERNAL_PROPERTY_ID_SPARSE, EMBR_INTERNAL_PROPERTY_ID_SPARSE2)(__VA_ARGS__)


#define EMBR_PROPERTIES_BEGIN \
struct id : embr::property::v1::tag::property_owner  \
{\
    template <int id_, bool = true> struct lookup;   \
    template <class T, int id_ = -1>             \
    using traits = embr::internal::property::v1::traits_base<this_type, T, id_>;

#define EMBR_PROPERTIES_SPARSE_BEGIN EMBR_PROPERTIES_BEGIN

#define EMBR_PROPERTIES_SPARSE_END };
#define EMBR_PROPERTIES_END } fields_;

#define EMBR_INTERNAL_PROPERTY_GETTER(type_, name_, alias_)   \
    constexpr type_ alias_() const { return base_type::template getter<typename impl_type::id::name_>(); }

#define EMBR_INTERNAL_PROPERTY_SETTER(type_, name_, alias_)   \
    void alias_(const type_& v)  \
{ base_type::template setter<typename impl_type::id::name_>(v); }

#define EMBR_INTERNAL_PROPERTY_ALIAS(type_, name_, alias_) \
    EMBR_INTERNAL_PROPERTY_GETTER(type_, name_, alias_)       \
    EMBR_INTERNAL_PROPERTY_SETTER(type_, name_, alias_)

#define EMBR_PROPERTY_ALIAS(name_, alias_) \
        EMBR_INTERNAL_PROPERTY_ALIAS(typename impl_type::id::name_::value_type, name_, alias_)

#define EMBR_PROPERTY(name_)    EMBR_PROPERTY_ALIAS(name_, name_)

//#define EMBR_PROPERTY(...) GET_MACRO(__VA_ARGS__, EMBR_PROPERTY_ID2_2, EMBR_PROPERTY_ID2_1)(__VA_ARGS__)


// --- keeping this around just incase
//    using typename base_type::impl_type; // this used to live in EMBR_PROPERTY_RUNTIME_BEGIN

// base_ parameter is needed to designate parent container of runtime.  Oftentimes this is
// embr::service::v1::Service
// service_core_exp_type is for experiments with autostart
#define EMBR_PROPERTY_RUNTIME_BEGIN(base_) \
typedef this_type impl_type;               \
typedef this_type service_core_exp_type;   \
template <class TSubject, class TImpl = this_type>  \
class runtime : public base_::runtime<TSubject, TImpl> \
{                                          \
    typedef runtime this_type;             \
    typedef base_::runtime<TSubject, TImpl> base_type; \
protected:                                  \
    using base_type::notify;                \
    \
public:\
    ESTD_CPP_FORWARDING_CTOR(runtime)

#define EMBR_PROPERTY_RUNTIME_END   };

// FIX: Not quite ready yet
#define EMBR_PROPERTY_INTERNAL_FIRE_CHANGE_ALIAS   \
    template <class TTraits, class T> inline void fire_changed(const T& v)  \
    { base_type::template fire_changed<TTraits>(v); }                       \
    template <class TTraits, class T> inline void fire_changing(const T& v_old, const T& v)  \
    { base_type::template fire_changing<TTraits>(v_old, v); }


// Not used yet, there's a descrepency on whether we need typename in there
#define EMBR_PROPERTY_DEFINE_STATIC_TYPE    \
    template <class TSubject>               \
    using static_type = typename static_factory<TSubject, this_type>::static_type;
