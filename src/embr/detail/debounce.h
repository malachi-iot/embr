#pragma once

#include <estd/chrono.h>

namespace embr { namespace detail {

namespace internal {

struct DebouncerBase
{
public:
    enum States
    {
        Unstarted = -1,
        Off,
        On
    };

    enum Substates
    {
        Idle,       ///< no on/off state evaluation happening
        EvalOn,     ///< last state evaluated was on
        EvalOff,    ///< last state evaluated was off
    };

private:
    // DEBT: May want to put this struct in Debouncer itself for better packing
    struct
    {
        States state_ : 3;
        Substates substate_: 3;
    };

protected:
    void state(States v) { state_ = v; }
    void state(Substates v) { substate_ = v; }

    DebouncerBase() : state_(States::Unstarted) {}
    DebouncerBase(bool on) : state_(on ? On : Off), substate_(Idle) {}

public:
    States state() const { return state_; }
    Substates substate() const { return substate_; }

    //void reset(States s = States::Unstarted)
    void reset()
    {
        //state(s);
        state(Idle);
    }
};

namespace impl {

template <typename TDuration = estd::chrono::duration<uint8_t, estd::milli>, unsigned threshold_ms = 40 >
struct Debouncer
{
    typedef TDuration duration;

    /// amount of "time energy" accumulated necessary to indicate a signal
    ESTD_CPP_CONSTEXPR_RET static duration signal_threshold()
    {
        return estd::chrono::milliseconds(threshold_ms);
    }

    /// Delta window in which debounce operates before auto resetting
    ESTD_CPP_CONSTEXPR_RET static duration max()
    {
        return estd::chrono::milliseconds(150);
    }
};

}

// DEBT: Consider bitmask approach rather than tracking time directly.  Pretty similar either way
template <class TImpl = impl::Debouncer<> >
class Debouncer : public DebouncerBase,
    TImpl
{
    typedef DebouncerBase base_type;
    typedef TImpl impl_type;
    typedef typename TImpl::duration duration;

private:
    // Amount of time observing blips into opposite state.
    // May or may not be noise.
    // If it passes signal_threshold, then we consider it
    // an actual indicator of signal.
    duration noise_or_signal;

    /// record amount of time spent in current eval state.  If time exceeds
    /// threshold, change to 'switch_to'
    /// @param delta
    /// @param switch_to
    /// @return
    bool encountered(duration delta, States switch_to);

    bool remove_energy(const duration& delta);

    bool time_passed_internal(duration delta, bool on);

public:
    ///
    /// @param delta
    /// @param on
    /// @return true = main state changed
    template <typename TRep, typename TPeriod>
    bool time_passed(const estd::chrono::duration<TRep, TPeriod>& delta, bool on);

    Debouncer() : base_type() {}
    Debouncer(bool on) : base_type(on) {}
};

}

typedef internal::Debouncer<> Debouncer;

}}