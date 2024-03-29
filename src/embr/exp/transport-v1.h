#pragma once

namespace embr { namespace experimental { inline namespace v1 {

namespace tags {

}

enum Transports
{
    TRANSPORT_SPI,
    TRANSPORT_UART,
    TRANSPORT_CAN
};

enum Support
{
    SUPPORT_MUST,
    SUPPORT_SHOULD,
    SUPPORT_MAY,
    SUPPORT_SHOULD_NOT,
    SUPPORT_MUST_NOT,

    SUPPORT_REQUIRED = SUPPORT_MUST,
    SUPPORT_OPTIONAL = SUPPORT_MAY,
    NOT_SUPPORTED = SUPPORT_MUST_NOT
};


enum TransportTraits
{
    TRANSPORT_TRAIT_CONTIGUOUS  = 0x0001,
    TRANSPORT_TRAIT_ASYNC       = 0x0008,
    TRANSPORT_TRAIT_BLOCKING    = 0x0010,
    TRANSPORT_TRAIT_POLLED      = 0x0020,
    TRANSPORT_TRAIT_TIMEOUT     = 0x0040,
    TRANSPORT_TRAIT_TRANSACTED  = 0x0080,
    TRANSPORT_TRAIT_PBUF        = 0x0100,
    TRANSPORT_TRAIT_FRAME       = 0x0200,
    TRANSPORT_TRAIT_ADDR_IN_FRAME = 0x0400,
};


struct transport_traits_defaults
{
    static constexpr Support contiguous = SUPPORT_REQUIRED;
    static constexpr Support non_contiguous = NOT_SUPPORTED;
    static constexpr Support callback_tx = NOT_SUPPORTED;
    static constexpr Support callback_rx = NOT_SUPPORTED;
    static constexpr Support callback_status = NOT_SUPPORTED;
    static constexpr Support global = SUPPORT_REQUIRED;
    static constexpr Support instanced = NOT_SUPPORTED;
    static constexpr Support async = NOT_SUPPORTED;
    static constexpr Support blocking = SUPPORT_OPTIONAL;
    static constexpr Support timeout = SUPPORT_OPTIONAL;
    static constexpr Support polled = SUPPORT_OPTIONAL;
    static constexpr Support transactional = NOT_SUPPORTED;
    static constexpr Support pbuf = NOT_SUPPORTED;
    static constexpr Support frame = NOT_SUPPORTED;
    static constexpr Support addr_in_frame = NOT_SUPPORTED;
    static constexpr Support bus_reserve = NOT_SUPPORTED;
    static constexpr Support thread_safe = NOT_SUPPORTED;
    static constexpr Support per_byte = NOT_SUPPORTED;
    static constexpr Support session = NOT_SUPPORTED;
    static constexpr Support addressed = NOT_SUPPORTED;

    // Undecided
    static constexpr Support static_transaction = NOT_SUPPORTED;
    static constexpr Support callback_global = SUPPORT_OPTIONAL;
    static constexpr Support callback_instance = SUPPORT_SHOULD;
};


template <class Transport>
struct transport_traits : transport_traits_defaults {};


template <Transports t, TransportTraits tr, class Variant = void, class enabled = void>
struct transport;

template <class Transport, TransportTraits>
struct mode;


// Nifty, but probably not gonna work:
// Even though we can compile time map our errors to their errors,
// transport A mapping of error 1 may have slightly different meaning
// than transport B mapping of error 1
enum ReturnStatus
{
    TRANSPORT_RET_OK,
    TRANSPORT_RET_TIMEOUT,
};


template <class Transport, ReturnStatus>
struct error;


}}}