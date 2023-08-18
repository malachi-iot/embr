#pragma once

namespace embr { namespace esp_idf { namespace event {

inline namespace v1 {

template <typename TEventId>
struct runtime
{
    const TEventId id;
    const void* const data;

    constexpr runtime(TEventId id, void* data) : id{id}, data{data} {}
};


namespace internal {

// NOTE: Consider changing this to 'bridge' since that's what it really does,
// bridges esp-idf events to embr events
template <const esp_event_base_t&>
struct handler;

// monostate means no type at all - vs void would be void*
template <typename TEventId, TEventId id_>
struct mapping { typedef estd::monostate type; };

template <typename TEventId, TEventId id_>
using mapping_t = typename mapping<TEventId, id_>::type;

template <class T>
struct data_base
{
    using data_type = T;
    using pointer = const data_type*;

    pointer const data;

    constexpr data_base(pointer data) : data{data} {}

    pointer operator->() const { return data; }
};

template <>
struct data_base<estd::monostate>
{
    using pointer = const void*;

    constexpr data_base(pointer) {}
};



}



template <typename TEventId, TEventId id_>
struct base : internal::data_base<internal::mapping_t<TEventId, id_>>
{
    typedef internal::data_base<internal::mapping_t<TEventId, id_>> base_type;

    using typename base_type::pointer;
    
    typedef TEventId event_type;
    static constexpr event_type id() { return id_; }

    constexpr base(const void* data) :
        base_type(static_cast<pointer>(data))
    {}

    // Disabling this since doing this will attempt to double up from the
    // explicit 'runtime' event already fired, plus it's incompatible with
    // no-data variants
    /*
    constexpr operator runtime<TEventId> () const
    {
        return runtime<TEventId>{id_, base_type::data};
    } */
};

}

inline namespace v2 {

#if __cpp_nontype_template_parameter_auto
template <auto event_id_>
using base = event::v1::base<decltype(event_id_), event_id_>;
#endif

}

}}}