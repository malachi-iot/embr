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
};

namespace impl {

template <typename TDuration =
#if ESTD_ARCH_BITNESS < 32
    // Gives us ~ 255ms max threshold in ms resolution
    estd::chrono::duration<uint8_t, estd::milli>,
#else
    // Gives us ~ 60ms max threshold in us resolution
    estd::chrono::duration<uint16_t, estd::micro>,
#endif
    unsigned threshold_ms = 40 >
struct Debouncer
{
    typedef TDuration duration;

    /// amount of "time energy" accumulated necessary to indicate a signal
    ESTD_CPP_CONSTEXPR_RET static duration signal_threshold()
    {
        return estd::chrono::milliseconds(threshold_ms);
    }

    /// Delta window in which debounce operates before auto resetting
    ESTD_CPP_CONSTEXPR_RET static estd::chrono::milliseconds max()
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

public:
    typedef typename TImpl::duration duration;

private:
    typedef typename duration::rep rep_type;

    struct
    {
        // This is a tricky stunt we pull.  estd::chrono::duration are always comprised of underlying rep
        // type.  So, we carefully allocate THAT here since we expect it to be intrinsic.  With that, structure
        // packing has no impediments to doing its thing.  For bonus points, in the odd circumstance where
        // chrono wants to take a pointer, we are good to go because this is the first variable in Debouncer
        // and is always aligned.
        rep_type noise_or_signal_;

        States state_ : 3;
        Substates substate_: 3;
        int unused_test_ : 2;
    };
    // NOTE: Doing this indeed packs things more, but risks noise_or_signal_ being off the proper boundary
    // in an array.  IIRC there are other GCC/C++ directives to specify alignment to force that back though.
    //__attribute__((packed));

    // Amount of time observing blips into opposite state.
    // May or may not be noise.
    // If it passes signal_threshold, then we consider it
    // an actual indicator of signal.
    inline duration& noise_or_signal() { return (duration&) noise_or_signal_; }


protected:
    void state(States v) { state_ = v; }
    void state(Substates v) { substate_ = v; }

public:
    Debouncer() : state_(States::Unstarted) {}
    Debouncer(bool on) : state_(on ? On : Off), substate_(Idle) {}

    // Mostly for debug curiosity
    inline const duration& noise_or_signal() const { return (duration&) noise_or_signal_; }

    States state() const { return state_; }
    Substates substate() const { return substate_; }

    //void reset(States s = States::Unstarted)
    void reset()
    {
        //state(s);
        state(Idle);
    }
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
}   __attribute__((packed));

}

typedef internal::Debouncer<> Debouncer;

}}