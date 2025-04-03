# BLE data types

Document v0.1

# Overview

Focuses on code which is cross platform/stack friendly.

At this time that is limited to GATT data types

Cross-platform work involving different stack and OS specific code is NOT RECOMMENDED here

After much deliberation whether to put this code here ot pgqt-util, `embr` felt right because ALL targets can use embr, but not all targets can use Qt

# Terminology

| Term      | Context   | Description
| -         | -         | - 
| BR/EDR    |           | ? relates to SDP
| L2CAP CoC |           | L2CAP Connection-oriented Channel
| OTS       |           | Object Transfer Service
| OLCP      | OTS       | Object List Control Point [1.1]
| OACP      | OTS       | Object Action Control Point [2]
| SDP       |           | Service Discovery Protocol
| SM        |           | ?

# References

1. https://www.bluetooth.com/specifications/specs/object-transfer-service-1-0/
    1. OTS_v10.pdf
2. https://www.esp32.com/viewtopic.php?t=6141