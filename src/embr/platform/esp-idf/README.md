# ESP32 esp-idf Support

Tested against v4.4.2

## Timer Scheduler

Fully ISR based

### Performance

With (most) logging disabled, we see microsecond counts in the ISR typically in the double digits
during debouncer usage.

## Debouncer

Fully ISR based
Relies heavily on Timer Scheduler

## Notes

## Debt

All these debug entries make the ISR go very slow, not unusual for it to exceed 60ms in the ISR!!!!!!

# References

1. https://docs.espressif.com/projects/esp-idf/en/v4.4.2/esp32/api-reference/system/perfmon.html
2. https://docs.espressif.com/projects/esp-idf/en/v4.4.2/esp32/api-reference/peripherals/timer.html