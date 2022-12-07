#pragma once

namespace embr { namespace experimental {


enum class transport_support
{
    preferred,
    native,
    emulated,
    unsupported
};

namespace tags {

template <transport_support s>
struct _support_level
{
    static constexpr transport_support support_level() { return s; }
};


// Connection oriented can co-exist with datagram
struct connection {};
struct datagram {};
struct stream : connection {};

// Not sure what we're gonna do about broadcast, but tagging it here anyway
struct broadcast {};

struct read_sequential {};
struct read_blocking : read_sequential {};
struct read_polling : read_sequential {};
struct read_callback {};
struct read_transaction {};

struct write_sequential {};
struct write_blocking : write_sequential {};
struct write_polling : write_sequential {};
struct write_callback {};
struct write_transaction {};
struct write_fire_and_forget {};


// buffer tags

struct buf_chained {};              // Whether buffer can be non-contiguous (usually a singly linked list)

// I expect we may have to make these runtime...
struct buf_system_allocated {};     // allocs happen either directly by transport system or through it
struct buf_user_allocated {};       // allocs happen more under user control such as malloc, free, etc.
struct buf_fixed_allocated {};      // allocs are nonexistent - static buffer
struct buf_runtime_allocated {};    // Any of the above 3 can be true

}

// because specialization on values wants integral types
enum monostate_enum {};

template <class TNativeTransport>
struct protocol_traits
{
    typedef monostate_enum type;

    static constexpr type def() { return monostate_enum{}; }
};


template <class TNativeTransport>
struct transport_traits;

template <class TTraits, typename TTraits::protocol_type v>
struct transport_traits_wrapper;

template <class TNativeTransport, typename protocol_traits<TNativeTransport>::type v
    //= monostate_enum{}>
    = protocol_traits<TNativeTransport>::def()>
    
struct transport_traits2;

template <class TNativeBuffer>
struct buffer_traits;

// DEBT: Move out to enum.h if we can forward an enum too
enum class transport_results
{
    OK = 0,
    TransportErrorBoundary = 100,
    Memory = TransportErrorBoundary,
    Route,
    Transport,
    AlreadyConnecting,
    AlreadyConnected,
    Reset,
    Timeout,
    Buffer,

    EmbrErrorBoundary = 1000,
    Undefined
};


template <typename TNativeTransport, class TTraits = transport_traits<TNativeTransport> >
struct Transport;

template <typename TResult>
transport_results unify_result(TResult);

// Experimenting with post_calculate for optimization - may not matter at all
template <typename TResult, bool post_calculate = false>
struct transport_result_wrapper;


template <typename TResult>
struct transport_result_wrapper<TResult, false>
{
    const transport_results result;

    transport_result_wrapper(TResult r) : result(unify_result(r)) {}

    operator transport_results() const { return result; }
};



template <typename TResult>
struct transport_result_wrapper<TResult, true>
{
    const TResult result;

    constexpr transport_result_wrapper(TResult r) : result(r) {}

    operator transport_results() const { return unify_result(result); }
};


}}
