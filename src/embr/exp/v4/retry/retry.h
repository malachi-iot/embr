#pragma once

#include <estd/chrono.h>
#include <estd/queue.h>
#include <estd/unordered_map.h>

#include "fwd.h"
#include "reference.h"

namespace embr { namespace experimental { inline namespace v4 {

// DEBT: Consider some kind of consolidation of unordered_map type here
template <
    unsigned N, class Endpoint, class Tracked,
    class Hash = estd::hash<Endpoint>, class Clock = estd::chrono::steady_clock>
struct RetryImpl
{
    using endpoint_type = Endpoint;
    using tracked_type = Tracked;
    using clock_type = Clock;
    using hasher = Hash;
    static constexpr unsigned count = N;
};


#define FEATURE_EMBR_RETRY_V4_ACK_IS_GC 0

// DEBT: Bring in and upgrade ebo helpers from embr::gl/embr::audio.  We want either:
// a) regular value here as tracked_ instance variable
// b) if tracked_type is empty struct do a value_evaporator style thing here
// In other words, we never really want only to extend from Impl::tracked_type
template <class Impl>
struct RetryItemBase : Impl::tracked_type
{
    using base_type = typename Impl::tracked_type;

    ESTD_CPP_FORWARDING_CTOR(RetryItemBase)
};

// TODO: Really we want tracked_type to be tracked_ or similar in here.  Reasons are:
// - reduced possibility for name collision
// - increased friendliness to packing
template <class Impl>
struct RetryItem : RetryItemBase<Impl>
{
    using base_type = RetryItemBase<Impl>;
    using clock_type = typename Impl::clock_type;
    using time_point = typename clock_type::time_point;
    using tracked_type = typename Impl::tracked_type;

//private:
    friend class Retry<Impl>;

    time_point next_attempt_;

    struct
    {
        // May or may not count original send, depending on consuming logic
        unsigned attempt_count_ : 4;
        // Ignored when FEATURE_EMBR_RETRY_V4_ACK_IS_GC is true
        unsigned ack_received_ : 1;
    };

    template <class ...Args>
    constexpr RetryItem(time_point next_attempt, Args&&...args) :
        base_type(std::forward<Args>(args)...),
        next_attempt_{next_attempt},
        attempt_count_{0},
        ack_received_{false}
    {

    }

//public:
    constexpr const time_point& next_attempt() const { return next_attempt_; }
    constexpr unsigned attempt_count() const { return attempt_count_; }
    constexpr bool ack_received() const { return ack_received_; }
    tracked_type& tracked() { return *this; }
};

template <class Impl>
class Retry : public Impl
{
    using base_type = Impl;

public:
    using typename base_type::endpoint_type;
    using typename base_type::tracked_type;
    using typename base_type::clock_type;
    using typename base_type::hasher;
    using base_type::count;

    using time_point = typename clock_type::time_point;
    using item_type = RetryItem<Impl>;
    using container_type = estd::layer1::unordered_map<endpoint_type, item_type, count, hasher>;
    using size_type = typename container_type::size_type;

    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using value_type = typename container_type::value_type;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;

private:
#if FEATURE_EMBR_RETRY_V4_ACK_IS_GC
    constexpr static bool is_null(const value_type& c)
    {
        return container_type::is_empty(c);
    }

    // FEATURE_EMBR_RETRY_V4_ACK_IS_GC doesn't need item_less, just keeping this around
    // as example of null handling
    struct item_less
    {
        constexpr bool operator()(const_pointer lhs, const_pointer rhs) const
        {
            if(is_null(*lhs)) return true;      // lhs == null is ALWAYS less than rhs
            if(is_null(*rhs)) return false;     // lhs != null is NEVER less than null rhs

            return lhs->second.next_attempt_ < rhs->second.next_attempt_;
        }
    };
#endif

    struct item_greater
    {
        constexpr bool operator()(const_pointer lhs, const_pointer rhs) const
        {
            return lhs->second.next_attempt_ > rhs->second.next_attempt_;
        }
    };

    container_type tracked_;

    // NOTE: Might be nice to track by just endpoint, but things like session IDs
    // which more comfortably live in tracked_type must be considered also
    estd::layer1::priority_queue<pointer, count, item_greater> next_;

    pointer pop();

    // gc as many 'top' items as we can - 'item' must be first 'top'
    void gc(pointer item);

    // retrack or untrack
    // MUST be 'top' item
    template <class F>
    void poll_one(pointer, F&&);

public:
    // NOT ACTIVE
    void track(const endpoint_type& endpoint, const tracked_type& tracked);

    template <class ...Args>
    pointer track(const endpoint_type& endpoint, time_point next_attempt, Args&&... args);

    // when transport receives an actual ACK, call this to remove him from retry tracking
    bool ack_received(const endpoint_type&);

    // Does not exclude ack'd ones when FEATURE_EMBR_RETRY_V4_ACK_IS_GC == 0
    constexpr size_type size() const { return tracked_.size(); }
    constexpr bool empty() const { return next_.empty(); }

    // DEBT: Make const-only, since this isn't needed for primary retry operations
    pointer top() const;

    // gc as many 'top' items as we can
    void gc();

    /// @brief poll_one investigate next ready, and if its time slot is up, call functor
    /// @param now
    /// @param f takes `pointer` parameter.  returns true if retrack is requested, false if done
    /// @return whether a ready item was processed
    /// @remarks
    template <class F>
    bool poll_one(time_point now, F&& f);

    ///
    /// @brief poll acts on every item who is ready now
    /// @param now
    /// @param f
    /// @return number of ready items processed
    template <class F>
    int poll(time_point now, F&& f);

    // If 'top' item has just been retried and now it's time to requeue for another,
    // call this guy
    void retrack(time_point next_attempt, bool gc = false);

    ///
    /// @brief Erase/GC 'top' (current) item if ACK was received
    /// @param force skip ACK check, untrack this no matter what
    /// @return true if we actually erased item, false otherwise
    /// @remarks consider naming 'untrack_top'
    bool untrack(bool force = false);

    // for 'top':
    // 1. gc (move active tracked item/pointer location)
    // 2. pop off priority_queue
    // 3. return newly moved pointer
    pointer gc_pop();

    // One-shot gc sweep through all of priority_queue
    void gc_sweep();

    /// is there an active retry ready to go now?
    /// @param now
    /// @return
    // DEBT: side effect gc's along the way
    pointer ready(time_point now);

    constexpr const item_type* operator[](const endpoint_type& e) const
    {
        const_iterator found = tracked_.find(e);

        if(found == tracked_.cend()) return nullptr;

        const value_type& p = *found;

        return &p.second;
    }
};

template <class Impl>
void Retry<Impl>::track(const endpoint_type& endpoint, const tracked_type& tracked)
{

}




}}}

