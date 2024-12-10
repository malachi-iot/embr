#pragma once

namespace embr {

struct void_subject
{
    /// @brief noop notify
    /// \tparam TEvent
    template <class Event>
    void notify(const Event&) const {}

    /// @brief noop notify
    /// \tparam TEvent
    /// \tparam TContext
    template <class Event, class Context>
    void notify(const Event&, Context&) const {}
};


}
