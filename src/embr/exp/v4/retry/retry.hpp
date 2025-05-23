#pragma once

#include "retry.h"

namespace embr { namespace experimental { inline namespace v4 {


template <class Impl>
template <class ...Args>
auto Retry<Impl>::track(const endpoint_type& endpoint, time_point next_attempt, Args&&... args) -> pointer
{
    estd::pair<iterator, bool> r = tracked_.try_emplace(endpoint, next_attempt, std::forward<Args>(args)...);

    if(!r.second) return nullptr;

    pointer i = r.first.value();

    next_.push(i);

    return i;
}

template <class Impl>
bool Retry<Impl>::ack_received(const endpoint_type& endpoint)
{
    iterator found = tracked_.find(endpoint);

    if(found == tracked_.cend())    return false;

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
auto Retry<Impl>::pop() -> pointer
{
    pointer item = next_.top();
    next_.pop();
    return item;
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
void Retry<Impl>::gc(pointer item)
{
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
        // Beware, this call asserts if erase fails
        //tracked_.gc_sparse_ll(item);

        next_.pop();
        if(next_.empty()) return;
    }
#endif
}

template <class Impl>
void Retry<Impl>::gc()
{
    if(next_.empty())   return;

    gc(next_.top());
}

template <class Impl>
void Retry<Impl>::retrack(time_point next_attempt, bool gc)
{
    //pointer item = next_.top();

    // Remember, gc_pop is gonna deep-copy swap, so only do gc with smaller
    // item_type
    pointer item = gc ? gc_pop() : pop();

    ++item->second.attempt_count_;
    item->second.next_attempt_ = next_attempt;
    next_.push(item);
}

template <class Impl>
bool Retry<Impl>::untrack(bool force)
{
    pointer item = next_.top();
    // DEBT: our clever traditional_accessor creates friction here
    //auto control = reinterpret_cast<control_pointer>(item);

#if FEATURE_EMBR_RETRY_V4_ACK_IS_GC
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
#else
    // Only untrack after an ACK is received unless forced to do otherwise
    if(!force && !item->second.ack_received_) return false;

    iterator it(&tracked_, item);

    tracked_.erase(it);
    next_.pop();
#endif

    return true;
}

template <class Impl>
auto Retry<Impl>::ready(time_point now) -> pointer
{
    gc();

    if(next_.empty()) return nullptr;

    pointer i = next_.top();

    return now >= i->second.next_attempt_ ? i : nullptr;
}


template <class Impl>
template <class F>
void Retry<Impl>::poll_one(pointer r, F&& f)
{
    if(f(r))
        retrack(r->second.next_attempt_);
    else
        // Force untrack if we're top and functor says we're done
        untrack(true);
}

template <class Impl>
template <class F>
void Retry<Impl>::poll_one(time_point now, F&& f)
{
    pointer r = ready(now);

    if(r == nullptr) return;

    poll_one(r, std::forward<F>(f));
}


template <class Impl>
template <class F>
void Retry<Impl>::poll(time_point now, F&& f)
{
    for(pointer r = ready(now); r != nullptr; r = ready(now))
        poll_one(r, std::forward<F>(f));
}


}}}

