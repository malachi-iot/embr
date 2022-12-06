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

template <class TNativeTransport>
struct transport_traits;


template <class TNativeBuffer>
struct buffer_traits;

enum class transport_results
{
    OK = 0,
    MemoryError,
    RouteError,
    TransportError
};


template <typename TNativeTransport, class TTraits = transport_traits<TNativeTransport> >
struct Transport;


}}
