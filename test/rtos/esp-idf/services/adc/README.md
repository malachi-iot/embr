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

PGESP-41 tells us *yes* it is an `xRingbuffer` and its natural usage is not lossy, therefore it will block/reject on full - thus conforming to aforementioned descriptions.

## 3.2. Necessity of adc_continuous_read()

`adc_continuous_read()` is required for conversion completion callbacks to continue unimpeded, in context of Section 3.1.'s ring buffer.

"If the generated results fill up the internal pool, new generated results will be lost." [1.1]

"When the pool is full, a pool overflow event will emerge. Under this condition, the driver won’t fill in the event data." [1]

The "internal pool" is a 2nd order buffer (xRingbuffer) as specified by max_store_buf_size.  If using DMA directly, one can ignore the internal pool.

## 3.3. Zero-copy

Callbacks operate directly on the DMA buffers and are only implicitly related to the "internal pool".  Therefore we can use these directly in a zero-copy capacity.  As per PGESP-41 Section 3.1 there's a hardcoded internal DMA page count of 5.

# References

1. https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/api-reference/peripherals/adc_continuous.html
    1. https://docs.espressif.com/projects/esp-idf/en/v5.1.2/esp32/api-reference/peripherals/adc_continuous.html#read-conversion-result
2. https://github.com/espressif/esp-idf/issues/4542
3. https://github.com/espressif/esp-idf/issues/12490
4. https://www.esp32.com/viewtopic.php?t=27606