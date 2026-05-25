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

// 23MAY26 - Do we need to start considering a 'Traits' instead of options & align_to?
template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf,
    // NOTE: Defaulting align_to somewhat aggressively.  Technically this probably ought to be
    // alignof(std::max_align_t) but that can be pretty big.
    msg_bipbuf_options o = MBB_OPT_NONE, unsigned align_to = sizeof(unsigned)>
class msg_bipbuf;

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf, class Traits>
class msg_bipbuf_exp;

// Very limited scope crude, explicit constexpr log2 to assist with bit size deduction
// Despite crudeness, pretty useful.  Consider putting this guy up into estd
constexpr int int_log2(uint32_t x, int r = -1)  // NOLINT
{
    return x ? int_log2(x >> 1, r + 1) : r;
}

template <ESTD_CPP_CONCEPT(estd::concepts::v1::Bipbuf) Buf,
    msg_bipbuf_options o, unsigned align_to>
class msg_bipbuf
{
public:
    static constexpr bool aligned = !(o & MBB_OPT_UNALIGNED);
    static constexpr bool retain_size = o & MBB_OPT_PRECISE_SIZE;

    //static constexpr unsigned align_size = sizeof(void*);
    //static constexpr unsigned align_log2 = int_log2(align_size);

    struct message_unaligned
    {
        const uint16_t sz;

        char payload[];
    };

    class alignas(align_to) header_aligned_extended // NOLINT
    {
        struct
        {
            unsigned sz;
            // For very big alignment requirements, bit packing gets awkward
        };
    };

    class alignas(align_to) header_aligned
    {
    protected:
        //static constexpr unsigned align_size = alignof(message_unaligned);
        // DEBT: No estd equivalent yet
        static constexpr unsigned align_size = align_to;
        static constexpr unsigned align_log2 = int_log2(align_size);

        //static_assert(align_size == 0);

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

    public:
        constexpr explicit header_aligned(unsigned size_in_bytes, bool ready) :
            //sz{retain_size ? size_in_bytes : size_in_bytes >> align_bits},
            sz{size_in_bytes},
            ready{ready}
        {
            //assert(size_in_bytes % align_size == 0);
        }

        constexpr explicit header_aligned(estd::nullopt_t) :
            sz{0},
            ready{true}
        {}
    };

    class alignas(align_to) message_aligned : public header_aligned
    {
        using base_type = header_aligned;
        using base_type::sz;

    public:
        template <class ...Args>
        constexpr explicit message_aligned(Args&&...args) :
            header_aligned(std::forward<Args>(args)...)
        {}

        void* payload() { return this + 1; }
        constexpr const void* payload() const { return this + 1; }

        static constexpr unsigned size(unsigned payload_size)
        {
            //static_assert(sizeof(message_aligned) == align_to);
            return sizeof(message_aligned) + payload_size;
        }

        /// In bytes
        constexpr unsigned payload_size() const
        {
            //return retain_size ? sz : sz << align_bits;
            return sz;
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
    template <class F, ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = internal::noop_mutex>
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


    template <class T, class ...Args, ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = noop_mutex>
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

    template <class F, ESTD_CPP_CONCEPT(internal::concepts::Mutex) Mutex = internal::noop_mutex>
    estd::errc pop(Mutex&& mutex, F&& f)
    {
        // DEBT: Works well enough, but peek may be doing a little more
        // than we need right now
        // DEBT: We don't get alignment warnings, but we'd kind of expect it here

        //if(!mutex.lock())   return estd::errc::no_lock_available; // See below for why we don't need locks here
        auto m = (message*)buf_.peek(sizeof(message));
        //mutex.unlock();

        // FIX: there is no non-const peek yet but we do need one
        //auto m = reinterpret_cast<message*>(buf_.peek(sizeof(message)));

        // See https://malachi.atlassian.net/wiki/x/AYClD for breakdown of why
        // peek is lock free.  In short, only 'pop' changes a_start, and only
        // `offer_end` changes a_end. a_end only ever changes from a is-empty condition
        // to a not is-empty condition.  Meaning that our empty check can
        // conservatively keep reporting 'nothing yet' until something's there.

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
