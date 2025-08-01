#pragma once

#include <estd/queue.h>

#include "../mutex.h"

#if __cpp_lib_concepts
#include <concepts>
#endif


namespace embr { namespace scheduler { inline namespace v1 {

#if __cpp_lib_concepts
namespace concepts {

template <class T>
concept Item = requires(T t)
{
    typename T::time_point;
};

template <class T>
concept Traits = requires
{
    typename T::value_type;
    typename T::const_reference;
    typename T::time_point;
    typename T::compare;
    //typename T::time_point T::next(std::declval<typename T::value_type>());
};

}
#endif

// DEBT: Seems like time_point really ought to consolidate here too
template <class Item>
struct item_traits
{
    using time_point = typename Item::time_point;

    ESTD_CPP_STD_VALUE_TYPE(Item)

    struct compare
    {
        constexpr bool operator()(const_pointer lhs, const_pointer rhs) const
        {
            assert(lhs);
            assert(rhs);

            return lhs->next() > rhs->next();
        }
    };

    static constexpr time_point next(const_reference v) { return v.next(); }
};


namespace detail {

struct scheduler_base
{
    enum process_result
    {

    };
};

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
class scheduler : public scheduler_base
{
    // DEBT: Deviating from original noop_mutex pattern in that we might consider baking
    // mutex context into the passed in mutex (noop_mutex is 1/2 way like a traits right now)
    using noop_mutex = internal::noop_mutex;

public:
    using traits = Traits;

    ESTD_CPP_STD_VALUE_TYPE(typename traits::value_type)

protected:
    estd::priority_queue<pointer, Container, typename traits::compare> items_;

public:
    using time_point = typename traits::time_point;

    ///
    /// @param now
    /// @return true if item was processed and scheduled again, false otherwise
    template <class Mutex = noop_mutex>
    bool process_one(time_point now, Mutex = {});
    void process(time_point now);

    template <class Mutex = noop_mutex>
    bool reschedule(pointer, Mutex = {});

    // Not mutex protected below this line, be careful

    constexpr bool empty() const { return items_.empty(); }

    constexpr const_reference top() const { return *items_.top(); }

    constexpr time_point next() const
    {
        return traits::next(*items_.top());
    }

    // NOTE: More echoes and overlap with scheduler
    constexpr bool ready(time_point now) const
    {
        return items_.empty() ? false : now >= next();
    }
};


}

}}}

// DEBT: Too-global v1 gonna have issues
namespace embr { namespace layer1 { inline namespace v1 {

template <ESTD_CPP_CONCEPT(embr::scheduler::concepts::Item) Item, unsigned N,
    ESTD_CPP_CONCEPT(embr::scheduler::concepts::Traits) Traits = embr::scheduler::item_traits<Item>>
using scheduler = embr::scheduler::detail::scheduler<Traits, estd::layer1::vector<Item*, N>>;

}}}
