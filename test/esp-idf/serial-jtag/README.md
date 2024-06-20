
# 1. Design Goals

# 2. Infrastructure

# 3. Opinions & Observations

See
https://www.esp32.com/viewtopic.php?t=38971

For (more undocumented, similar to TinyUSB, thanks) ways to use the ESP32C6 (and friends) USB CDC/ACM peripheral

Section 31 of datasheet [1] goes into detail on the limited USB peripheral

Seems that one can't inhibit log output on one console without inhibiting it on the other [2]

# 4. Conclusions

# References

1. https://www.espressif.com/sites/default/files/documentation/esp32-c6_technical_reference_manual_en.pdf
2. https://esp32.com/viewtopic.php?t=29237