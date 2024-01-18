# Overview

# 1. RESERVED

# 2. RESERVED

# 3. Observations

## 3.1. Presence of ring buffer

It's not officially stated, but it seems adc internal buffer is a ring buffer [3] [4]
I would wager it's that FreeRTOS ring buffer extension.

That said, there's conflicting statements:
"If the generated results fill up the internal pool, new generated results will be lost." [1.1]
"`adc_continuous_handle_cfg_t::max_store_buf_size` set the maximum size (in bytes) of the pool that the driver saves ADC conversion result into. If this pool is full, new conversion results will be lost."

Suggesting strongly it ISN'T a ring buffer

## 3.2. Necessity of adc_continuous_read()

It's unclear whether `adc_continuous_read()` is required for conversion completion callbacks to continue unimpeded.

"If the generated results fill up the internal pool, new generated results will be lost." [1.1] suggests NO.

"When the pool is full, a pool overflow event will emerge. Under this condition, the driver won’t fill in the event data." [1]

However, when I experimented with ADC callback a few months ago, I did not use that API and I seemed to get all the data.

If "pool" is a 2nd order buffer after the one specified by max_store_buf_size, that might clear up some things.

## 3.3. Zero-copy

Depending on further findings of 3.2. and 3.3. we might be able to do a zero-copy treatment, provided of coure that we are quick about it.

# References

1. https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/api-reference/peripherals/adc_continuous.html
    1. https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/api-reference/peripherals/adc_continuous.html#read-conversion-result
2. https://github.com/espressif/esp-idf/issues/4542
3. https://github.com/espressif/esp-idf/issues/12490
4. https://www.esp32.com/viewtopic.php?t=27606