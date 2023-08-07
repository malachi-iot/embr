#pragma once

#include <estd/internal/platform.h>
#include <estd/type_traits.h>

// Implementation of https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
// Adaptation of https://github.com/craftmetrics/esp32-button

namespace embr { namespace internal {

template <class>
struct DebounceButtonMask;

template <>
struct DebounceButtonMask<uint32_t> :
    estd::integral_constant<uint32_t, 0b1111000000111111> {};


template <class Unsigned>
class DebounceButtonHistoryBase
{
protected:
    Unsigned history;
    static CONSTEXPR Unsigned mask = DebounceButtonMask<Unsigned>::value;

public:
    ESTD_CPP_CONSTEXPR_RET EXPLICIT
    DebounceButtonHistoryBase() : history(0) {}

    void update(unsigned level)
    {
        history <<= 1;
        history |= level;
    }

    Unsigned masked() const
    {
        return history & mask;
    }
};


template <class Unsigned>
class DebounceButtonHistory;

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
            base_type::history = 0xFFFF;
            return true;
        }
        return false;
    }

    bool eval_off()
    {
        if(base_type::masked() == 0b1111000000000000)
        {
            base_type::history = 0;
            return true;
        }
        return false;
    }

    bool is_on() const
    {
        return base_type::history == 0xFFFF;
    }

    bool is_off() const
    {
        return base_type::history == 0;
    }
};



}}
