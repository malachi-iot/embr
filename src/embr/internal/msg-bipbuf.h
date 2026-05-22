#pragma once

#include <estd/internal/bip/buffer.h>
#include <estd/internal/optional.h>
#include <estd/internal/utility.h>
#include <estd/span.h>
#include <estd/system_error.h>

#include "mutex.h"

// A MPSC bit buffer with size-aware elements

namespace embr { namespace internal {

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf, class Mutex = noop_mutex>
class msg_bipbuf;

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf, class Mutex>
class msg_bipbuf : protected Mutex
{
public:
    struct message_unaligned
    {
        const uint16_t sz;

        char payload[];
    };

    struct message_aligned
    {
        struct
        {
            // Payload size:
            // * 4k on 16-bit (AVR)
            // * 512MB on 32-bit (ESP32)
            const unsigned sz : (sizeof(unsigned) * 8) - 4;
            
            // EXPERIMENTAL - condition where producer knows it's time consuming to fully
            // enqueue data, so you can reserve a certain size then set this flag later
            unsigned ready : 1;
        };

        constexpr explicit message_aligned(unsigned sz, bool ready) :
            sz{sz},
            ready{ready}
        {}

        constexpr explicit message_aligned(estd::nullopt_t) :
            sz{0},
            ready{true}
        {}

        void* payload() { return this + 1; }

        static constexpr unsigned size(unsigned payload_size)
        {
            return sizeof(message_aligned) + payload_size;
        }

        // Total size w/ payload
        constexpr unsigned size() const { return size(sz); }

        constexpr unsigned aligned_size() const { return size(sz); }

        estd::span<char> to_span() { return { (char*)payload(), sz }; }
    };

    using message = message_aligned;

private:
    Buf buf_;

public:
    msg_bipbuf() = default;

    template <class ...Args>
    constexpr msg_bipbuf(estd::in_place_t, Args&&...args) :
        buf_{std::forward<Args>(args)...}
    {}

    ///
    /// @param init - functor receiving 'message' object w/ payload() accessor
    /// @param sz - requested size of payload
    /// @param mutex
    /// @return
    ///     - no_lock_available: can't mutex lock
    ///     - not_enough_memory: out of bipbuf space
    ///     - {} == OK
    template <class F, class Mutex2 = Mutex>
    estd::errc push(F&& init, unsigned sz, Mutex2&& mutex = {})
    {
        if(!mutex.lock())  return estd::errc::no_lock_available;

        const unsigned msg_size = message::size(sz);

        if(buf_.unused() < msg_size)
        {
            mutex.unlock();
            return estd::errc::not_enough_memory;
        }

        auto m = (message*) buf_.offer_begin();

        new (m) message(sz, true);

        init(m);

        buf_.offer_end(msg_size);

        mutex.unlock();

        return {};
    }


    template <class T, class ...Args, class Mutex2 = embr::internal::noop_mutex>
    estd::errc emplace(Mutex2&& mutex, Args&&...args)
    {
        return push(
            [&](message* m)
            {
                new (m->payload()) T(std::forward<Args>(args)...);
            },
            sizeof(T), std::forward<Mutex2>(mutex));
    }

    template <class F, class Mutex2 = Mutex>
    estd::errc pop(F&& f, Mutex2&& mutex = {})
    {
        // DEBT: Works well enough, but peek may be doing a little more
        // than we need right now
        auto m = (message*)buf_.peek(sizeof(message));

        // See https://malachi.atlassian.net/wiki/x/AYClD for breakdown of why
        // peek is lock-free

        // DEBT: Use resource_unavailable_try_again once we have that
        // https://github.com/malachi-iot/estdlib/issues/201
        if(m == nullptr)    return estd::errc::no_buffer_space;

        f(m);

        // Awkward!  But at least f(m) runs so app part of dequeue isn't in a limbo
        // state
        // DEBT: Need a method like dequeue_poll() to recover from this
        // condition
        if(!mutex.lock())  return estd::errc::no_lock_available;

        // DEBT: Similar to peek, we really want a poll_end to streamline
        // these always-coupled operations
        buf_.poll(m->size());

        mutex.unlock();

        return {};
    }

    constexpr const Buf& buf() const { return buf_; }

    constexpr bool empty() const { return buf_.used() == 0; }
};

}}
