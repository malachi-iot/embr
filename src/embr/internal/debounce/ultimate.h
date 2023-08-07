#pragma once

#include <estd/limits.h>
#include <estd/type_traits.h>

#include <estd/internal/macro/push.h>

/**
 * Implementation of https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
 * Adaptation of https://github.com/craftmetrics/esp32-button
 *
 * General idea is this is a tri-state "pattern match" debouncer:
 * eval on: just noticed an ON condition
 * eval off: just noticed an OFF condition
 * undecided: mid-evaluation, neither condition noticed
 *
 * Specifics
 * eval on: repeats frequently as long as on condition is noticed
 * eval off: present only if preceded by state on
 *
 */



namespace embr { namespace internal {

template <class>
struct UltimateDebouncerTraits;


template <>
struct UltimateDebouncerTraits<uint8_t>
{
    static CONSTEXPR uint8_t mask_on = 0x07;        // 0b00000111
    static CONSTEXPR uint8_t mask_off = 0xC0;       // 0b11000000
    static CONSTEXPR uint8_t mask = mask_on | mask_off;
};

template <>
struct UltimateDebouncerTraits<uint16_t>
{
    static CONSTEXPR uint16_t mask_on = 0x003F;     // 0b0000000000111111;
    static CONSTEXPR uint16_t mask_off = 0xF000;    // 0b1111000000000000;
    static CONSTEXPR uint16_t mask = mask_on | mask_off;
};


template <class Unsigned, class Traits = UltimateDebouncerTraits<Unsigned> >
class DebounceButtonHistory
{
    typedef estd::numeric_limits<Unsigned> limits;
    typedef Traits traits_type;

protected:
    Unsigned history;

    Unsigned masked() const
    {
        return history & traits_type::mask;
    }

public:
    ESTD_CPP_CONSTEXPR_RET EXPLICIT
    DebounceButtonHistory() : history(0) {}

    void update(unsigned level)
    {
        history <<= 1;
        history |= level;
    }

    bool on() const
    {
        return history == limits::max();
    }

    bool off() const
    {
        return history == 0;
    }

    bool eval_on()
    {
        if(masked() == traits_type::mask_on)
        {
            history = limits::max();
            return true;
        }
        return false;
    }

    bool eval_off()
    {
        if(masked() == traits_type::mask_off)
        {
            history = 0;
            return true;
        }
        return false;
    }
};



// DEBT: Return an enum - 1 = on, 2 = off, 0 = still evaluating
template <class Unsigned>
unsigned eval(DebounceButtonHistory<Unsigned>& dbh)
{
    if(dbh.eval_on()) return 1;
    if(dbh.eval_off()) return 2;
    return 0;
}



}}


#include <estd/internal/macro/pop.h>
