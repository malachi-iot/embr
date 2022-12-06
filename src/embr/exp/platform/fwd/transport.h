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

struct read_blocking {};
struct read_callback {};
struct read_transaction {};

struct write_blocking {};
struct write_callback {};
struct write_transaction {};
struct write_fire_and_forget {};

struct buf_chained {};

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
