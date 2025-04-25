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
| BR/EDR    |           | More or less is Bluetooth Classic
| CID       | L2CAP     | Channel ID
| EATT      | BLE       | Enhanced Attribute Protocol (concurrency)
| HCI       | BLE       | Host Controller Interface
| HIDD      | BLE       | Host Interface Device Description (profile)
| HS        | NimBLE    | Host Stack
| L2CAP CoC |           | L2CAP Connection-oriented Channel
| LSO       |           | Least Significant Octet first (little endian)
| NPL       | NimBLE    | Nimble Porting Layer
| OTS       |           | Object Transfer Service
| OLCP      | OTS       | Object List Control Point [1.1]
| OACP      | OTS       | Object Action Control Point [2]
| PDU       |           | Protocol Data Unit
| PSM       | L2CAP     | Protocol/Service Multiplexer (identifies protocol)
| SDP       |           | Service Discovery Protocol
| SDU       |           | Service Data Unit (Application-level payload, especially with L2CAP CoC)
| SM        |           | ?

# References

1. https://www.bluetooth.com/specifications/specs/object-transfer-service-1-0/
    1. OTS_v10.pdf
2. https://www.esp32.com/viewtopic.php?t=6141