#pragma once

namespace embr {

struct void_subject
{
    /// @brief noop notify
    /// \tparam TEvent
    template <class TEvent>
    void notify(const TEvent&) const {}

    /// @brief noop notify
    /// \tparam TEvent
    /// \tparam TContext
    template <class TEvent, class TContext>
    void notify(const TEvent&, TContext&) const {}
};


}