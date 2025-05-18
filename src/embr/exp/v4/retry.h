#pragma once

#include <estd/queue.h>
#include <estd/unordered_map.h>

#include "retry/fwd.h"
#include "retry/reference.h"

namespace embr { namespace experimental { inline namespace v4 {

// DEBT: Consider some kind of consolidation of unordered_map type here
template <unsigned N, class Endpoint, class Tracked, class Clock = estd::chrono::steady_clock>
struct RetryImpl
{
    using endpoint_type = Endpoint;
    using tracked_type = Tracked;
    using clock_type = Clock;
    static constexpr unsigned count = N;
};


#define FEATURE_EMBR_RETRY_V4_ACK_IS_GC 1


template <class Impl>
struct RetryItem : Impl::tracked_type
{
    using base_type = typename Impl::tracked_type;
    using clock_type = typename Impl::clock_type;
    using time_point = typename clock_type::time_point;

    time_point next_attempt_;

    // Not counting original send
    unsigned retry_count_{};
    // Ignored when FEATURE_EMBR_RETRY_V4_ACK_IS_GC is true
    bool ack_received_{};

    ESTD_CPP_FORWARDING_CTOR(RetryItem)
};

template <class Impl>
class Retry : public Impl
{
    using base_type = Impl;
public:
    using typename base_type::endpoint_type;
    using typename base_type::tracked_type;
    using typename base_type::clock_type;
    using base_type::count;

    using time_point = typename clock_type::time_point;
    using item_type = RetryItem<Impl>;
    using container_type = estd::layer1::unordered_map<endpoint_type, item_type, count>;
    using size_type = typename container_type::size_type;

    using iterator = typename container_type::iterator;
    using value_type = typename container_type::value_type;
    using pointer = typename container_type::pointer;
    using const_pointer = typename container_type::const_pointer;

private:
    using control_type = typename container_type::control_type;
    using control_pointer = typename container_type::control_pointer;
    //using container_traits = typename container_type::traits;
    //using nullable = typename container_traits::nullable;
    using nullable = typename container_type::nullable;


    // DEBT: Because is_null_or_sparse/container::traits isn't exposed
    constexpr static bool is_null(const value_type& c)
    {
        return nullable::is_null(c.first);
    }

    struct item_less
    {
        constexpr bool operator()(const_pointer lhs, const_pointer rhs) const
        {
            if(is_null(*lhs)) return true;      // lhs == null is ALWAYS less than rhs
            if(is_null(*rhs)) return false;     // lhs != null is NEVER less than null rhs

            return lhs->second.next_attempt_ < rhs->second.next_attempt_;
        }
    };

    container_type tracked_;

    // NOTE: Might be nice to track by just endpoint, but things like session IDs
    // which more comfortably live in tracked_type must be considered also
    estd::layer1::priority_queue<pointer, count, item_less> next_;

public:
    void track(const endpoint_type& endpoint, const tracked_type& tracked);

    template <class ...Args>
    pointer track(const endpoint_type& endpoint, time_point next_attempt, Args&&... args);
    bool ack_received(const endpoint_type&);

    constexpr size_type size() const { return tracked_.size(); }

    pointer top();

    // gc as many 'top' items as we can
    void poll(time_point now);

    // If 'top' item has just been retried and now it's time to requeue for another,
    // call this guy
    void retrack(time_point next_attempt);

    // If 'top' item can be GC'd, do it via this method - needs better name
    bool untrack();

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
    bool is_ready(time_point now)
    {
        poll(now);

        if(next_.empty()) return false;

        pointer i = next_.top();

        return now >= i->second.next_attempt_;
    }
};

template <class Impl>
void Retry<Impl>::track(const endpoint_type& endpoint, const tracked_type& tracked)
{

}

template <class Impl>
template <class ...Args>
auto Retry<Impl>::track(const endpoint_type& endpoint, time_point next_attempt, Args&&... args) -> pointer
{
    estd::pair<iterator, bool> r = tracked_.try_emplace(endpoint, std::forward<Args>(args)...);

    if(!r.second) return nullptr;

    pointer i = r.first.value();

    i->second.next_attempt_ = next_attempt;

    next_.push(i);

    return i;
}

template <class Impl>
bool Retry<Impl>::ack_received(const endpoint_type& endpoint)
{
    iterator found = tracked_.find(endpoint);

    if(found == tracked_.cend())    return false;

    found->second.ack_received_ = true;

#if FEATURE_EMBR_RETRY_V4_ACK_IS_GC
    // unordered_map has a clever pseudo GC in it.  This means 'found' will linger
    // a bit longer.  Be advised this nulls out the endpoint/key also
    // NOTE: this nulling out might corrupt priority_queue, since 'null' entries
    // are less than non-null entries but still sitting in the middle.  Pushes
    // might get confused
    tracked_.erase(found);
#else
    found->second.ack_received_ = true;
#endif

    // gc is a combination of gc_sparse_ll and gc_active
    // gc_sparse_ll truly frees us from sparse -> null item
    // gc_active effectively reallocs on top of freed null spots

    return true;
}


template <class Impl>
void Retry<Impl>::gc_sweep()
{

}


template <class Impl>
auto Retry<Impl>::gc_pop() -> pointer
{
    iterator it{&tracked_, next_.top()};
    //auto item = reinterpret_cast<control_pointer>(next_.top());

    //item = tracked_.gc_active_ll(item);

    // TODO: Make a estd gc_active which takes a direct pointer too, or perhaps
    // expose above ll flavor
    it = tracked_.gc_active(it);
    next_.pop();

    return it.value();
}

template <class Impl>
auto Retry<Impl>::top() -> pointer
{
    if(next_.empty()) return nullptr;

    return next_.top();
}


template <class Impl>
void Retry<Impl>::poll(time_point now)
{
    if(next_.empty())   return;

    pointer item = top();

#if FEATURE_EMBR_RETRY_V4_ACK_IS_GC
    for(; is_null(*item); item = next_.top())
    {
        // do gc
        tracked_.gc_sparse_ll(item);

        next_.pop();
        if(next_.empty()) return;
    }
#else
    for(; item->second.ack_received_; item = next_.top())
    {
        iterator it{&tracked_, item};
        //auto cp = reinterpret_cast<control_pointer>(item);
        tracked_.erase(it);
        tracked_.gc_sparse_ll(item);

        next_.pop();
        if(next_.empty()) return;
    }
#endif
}

template <class Impl>
void Retry<Impl>::retrack(time_point next_attempt)
{
    //pointer item = next_.top();

    //next_.pop();
    // FIX: This is bad - technically works but we are gonna be memcpy'ing inline buffers
    // around too much
    pointer item = gc_pop();

    ++item->second.retry_count_;
    item->second.next_attempt_ = next_attempt;
    next_.push(item);
}

template <class Impl>
bool Retry<Impl>::untrack()
{
    pointer item = next_.top();
    // DEBT: our clever traditional_accessor creates friction here
    //auto control = reinterpret_cast<control_pointer>(item);

    // can't do erase_and_gc_ll because that guy likes to move
    // others around.  A little too low level for comfort here.
    // what this does is indicate to tracked_ that this slot truly
    // is null.  That presumes this WAS marked_for_gc, which presumes
    // a 'destroy' (i.e. erase) was previously called
    // https://github.com/malachi-iot/estdlib/issues/113
    //if(tracked_.is_null_or_sparse(*control))
    if(!is_null(*item)) return false;

    tracked_.gc_sparse_ll(item);
    //control->second.marked_for_gc = 0;
    return true;
}

}}}

