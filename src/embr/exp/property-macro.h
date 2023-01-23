#pragma once

#define EMBR_INTERNAL_PROPERTY_TRAITS_BODY(owner, type, id, desc) \
typedef event::traits_base<owner, type, id> base_type; \
typedef owner owner_type;                              \
using typename base_type::value_type;                  \
static constexpr const char* name() { return desc; }


#define EMBR_PROPERTY_TRAITS_GETTER_SETTER(host_, name_) \
    static constexpr value_type get(const host_& o)       \
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
    static constexpr owner_type& host(owner_type& o) { return o; } \
    static constexpr const owner_type& host(const owner_type& o) { return o; } \
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
    static constexpr struct id& host(this_type& o) { return o.fields_; } \
    static constexpr const struct id& host(const this_type& o) { return o.fields_; } \
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
