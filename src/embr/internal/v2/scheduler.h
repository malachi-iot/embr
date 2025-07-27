#pragma once

#include <estd/queue.h>

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
            return lhs->next() > rhs->next();
        }
    };

    static constexpr time_point next(const_reference v) { return v.next(); }
};


namespace detail {

template <ESTD_CPP_CONCEPT(concepts::Traits) Traits, class Container>
class scheduler
{
    using traits = Traits;

    ESTD_CPP_STD_VALUE_TYPE(typename traits::value_type)

    estd::priority_queue<pointer, Container, typename traits::compare> items_;

public:
    using time_point = typename traits::time_point;

    ///
    /// @param now
    /// @return true if item was processed and rescheduled, false otherwise
    bool process_one(time_point now);
    void process(time_point now);
    bool reschedule(pointer);

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

namespace embr { namespace layer1 { inline namespace v1 {

template <ESTD_CPP_CONCEPT(embr::scheduler::concepts::Item) Item, unsigned N,
    ESTD_CPP_CONCEPT(embr::scheduler::concepts::Traits) Traits = embr::scheduler::item_traits<Item>>
using scheduler = embr::scheduler::detail::scheduler<Traits, estd::layer1::vector<Item*, N>>;

}}}