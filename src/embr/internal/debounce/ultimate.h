#pragma once

#include <estd/limits.h>
#include <estd/type_traits.h>

#include <estd/internal/macro/push.h>

// Implementation of https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
// Adaptation of https://github.com/craftmetrics/esp32-button

// General idea is this is a tri-state "pattern match" debouncer:
// state on: just noticed an ON condition
// state off: just noticed an OFF condition
// undecided: mid-evaluation, neither condition noticed

namespace embr { namespace internal {

template <class>
struct DebounceButtonMask;

template <>
struct DebounceButtonMask<uint32_t> :
    estd::integral_constant<uint32_t, 0b1111000000111111> {};

template <>
struct DebounceButtonMask<uint8_t> :
    estd::integral_constant<uint8_t, 0b11000111> {};


template <class Unsigned, Unsigned mask_= DebounceButtonMask<Unsigned>::value>
class DebounceButtonHistoryBase
{
    typedef estd::numeric_limits<Unsigned> limits;

protected:
    Unsigned history;
    static CONSTEXPR Unsigned mask = mask_;

    void set_on()   { history = limits::max(); }
    void set_off()  { history = 0; }

    Unsigned masked() const
    {
        return history & mask;
    }

public:
    ESTD_CPP_CONSTEXPR_RET EXPLICIT
    DebounceButtonHistoryBase() : history(0) {}

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
};


template <class Unsigned>
class DebounceButtonHistory;

template <>
class DebounceButtonHistory<uint8_t> :
    public DebounceButtonHistoryBase<uint8_t>
{
    typedef DebounceButtonHistoryBase<uint8_t> base_type;

public:
    bool eval_on()
    {
        if(base_type::masked() == 0b00000111)
        {
            base_type::set_on();
            return true;
        }
        return false;
    }

    bool eval_off()
    {
        if(base_type::masked() == 0b11000000)
        {
            base_type::set_off();
            return true;
        }
        return false;
    }
};

template <>
class DebounceButtonHistory<uint32_t> :
    public DebounceButtonHistoryBase<uint32_t>
{
    typedef DebounceButtonHistoryBase<uint32_t> base_type;

public:
    bool eval_on()
    {
        if(base_type::masked() == 0b0000000000111111)
        {
            base_type::set_on();
            return true;
        }
        return false;
    }

    bool eval_off()
    {
        if(base_type::masked() == 0b1111000000000000)
        {
            base_type::set_off();
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
