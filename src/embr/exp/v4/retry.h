#pragma once

#include <estd/queue.h>
#include <estd/unordered_map.h>

#include "retry/fwd.h"
#include "retry/reference.h"

namespace embr { namespace experimental { inline namespace v4 {

// DEBT: Consider some kind of consolidation of unordered_map type here
template <unsigned N, class Endpoint, class Tracked = ReferenceTracked<>, class Clock = estd::chrono::steady_clock>
struct RetryImpl
{
    using endpoint_type = Endpoint;
    using tracked_type = Tracked;
    using clock_type = Clock;
    static constexpr unsigned count = N;
};


template <class Impl>
struct RetryItem : Impl::tracked_type
{
    using base_type = typename Impl::tracked_type;
    using clock_type = typename Impl::clock_type;
    using time_point = typename clock_type::time_point;

    // See https://github.com/malachi-iot/estdlib/issues/110
    //time_point last_attempt_;
    unsigned count_{};
    unsigned last_attempt_{};

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

private:
    using iterator = typename container_type::iterator;

    struct item_less
    {
        constexpr bool operator()(const item_type* lhs, const item_type* rhs) const
        {
            return lhs->last_attempt_ < rhs->last_attempt_;
        }
    };

    container_type tracked_;
    estd::layer1::priority_queue<item_type*, count, item_less> next_;

public:
    void track(const endpoint_type& endpoint, const tracked_type& tracked);

    template <class ...Args>
    bool track(const endpoint_type& endpoint, Args&&... args);
    bool ack_received(const endpoint_type&);

    constexpr size_type size() const { return tracked_.size(); }
};

template <class Impl>
void Retry<Impl>::track(const endpoint_type& endpoint, const tracked_type& tracked)
{

}

template <class Impl>
template <class ...Args>
bool Retry<Impl>::track(const endpoint_type& endpoint, Args&&... args)
{
    estd::pair<iterator, bool> r = tracked_.try_emplace(endpoint, std::forward<Args>(args)...);

    if(!r.second) return false;

    item_type& item = r.first->second;
    next_.push(&item);

    return true;
}

template <class Impl>
bool Retry<Impl>::ack_received(const endpoint_type& endpoint)
{
    iterator found = tracked_.find(endpoint);

    if(found == tracked_.cend())    return false;

    // TODO: unordered_map has a clever psuedo GC in it useful for just this scenario

    return true;
}


}}}

