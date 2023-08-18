#pragma once

namespace embr { namespace esp_idf { namespace event {

// monostate means no type at all - vs void would be void*
template <typename TEventId, TEventId id_>
struct mapping { typedef estd::monostate type; };

template <typename TEventId = int>
struct runtime
{
    const TEventId id;
    const void* const data;

    constexpr runtime(TEventId id, void* data) : id{id}, data{data} {}
};


namespace internal {


template <class T>
struct data_base
{
    using data_type = T;
    using pointer = const data_type*;

    pointer const data;

    constexpr data_base(pointer data) : data{data} {}
};

template <>
struct data_base<estd::monostate>
{
    using pointer = const void*;

    constexpr data_base(pointer) {}
};



}



template <typename TEventId, TEventId id_>
struct base : internal::data_base<typename mapping<TEventId, id_>::type>
{
    typedef internal::data_base<typename mapping<TEventId, id_>::type> base_type;

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


}}}