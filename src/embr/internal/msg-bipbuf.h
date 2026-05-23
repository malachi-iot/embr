#pragma once

#include <estd/internal/bip/buffer.h>
#include <estd/internal/optional.h>
#include <estd/internal/utility.h>
#include <estd/span.h>
#include <estd/system_error.h>

#include "mutex.h"
#include "fwd.h"

// A MPSC bit buffer with size-aware elements

namespace embr { namespace internal {

enum msg_bipbuf_options
{
    MBB_OPT_NONE            = 0x00,
    MBB_OPT_UNALIGNED       = 0x01,

    /// By default, msg_bipbuf is free to take action on underlying message size without retaining
    /// original size necessarily.  This flag ensures that original size really is retained.
    MBB_OPT_PRECISE_SIZE    = 0x02
};

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf, msg_bipbuf_options o = MBB_OPT_NONE>
class msg_bipbuf;

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf, msg_bipbuf_options o>
class msg_bipbuf
{
public:
    static constexpr bool aligned = !(o & MBB_OPT_UNALIGNED);
    static constexpr bool retain_size = o & MBB_OPT_PRECISE_SIZE;

    // Specifically to honor alignment
    static constexpr unsigned message_header_size = sizeof(void*);

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
    constexpr explicit msg_bipbuf(estd::in_place_t, Args&&...args) :
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
    template <class F, class Mutex = internal::noop_mutex>
    estd::errc push(Mutex&& mutex, F&& init, unsigned sz)
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


    template <class T, class ...Args, class Mutex = noop_mutex>
    estd::errc emplace(Mutex&& mutex, Args&&...args)
    {
        return push(
            std::forward<Mutex>(mutex),
            [&](message* m)
            {
                new (m->payload()) T(std::forward<Args>(args)...);
            },
            sizeof(T));
    }

    template <class F, class Mutex = internal::noop_mutex>
    estd::errc pop(Mutex&& mutex, F&& f)
    {
        // DEBT: Works well enough, but peek may be doing a little more
        // than we need right now
        // DEBT: We don't get alignment warnings, but we'd kind of expect it here
        auto m = (message*)buf_.peek(sizeof(message));
        // FIX: there is no non-const peek yet but we do need one
        //auto m = reinterpret_cast<message*>(buf_.peek(sizeof(message)));

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
