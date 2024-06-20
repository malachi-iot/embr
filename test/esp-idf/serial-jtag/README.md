# Overview

Support streambuf code for "baby" USB ACM/JTAG adapter present on ESP32C3, C6

# 1. Design Goals

Low level streambuf wrapper functional.

Any kind of extra buffering/intelligence is purely for testing and proof of concept.

# 2. Infrastructure

ESP32C6 DevKit used

# 3. Opinions & Observations

See
https://www.esp32.com/viewtopic.php?t=38971

For (more undocumented, similar to TinyUSB, thanks) ways to use the ESP32C6 (and friends) USB CDC/ACM peripheral

Section 31 of datasheet [1] goes into detail on the limited USB peripheral

Seems that one can't inhibit log output on one console without inhibiting it on the other [2]

# 4. Conclusions

## 4.1. ESP32C6

ESP-IDF v5.1.4 does not output, although input does seem to work
ESP-IDF v5.2.2 input and output works

# References

1. https://www.espressif.com/sites/default/files/documentation/esp32-c6_technical_reference_manual_en.pdf
2. https://esp32.com/viewtopic.php?t=29237