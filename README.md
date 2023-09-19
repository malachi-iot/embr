# embr

A monolithic library comprised of useful code for embedded oriented operations

## Platform-independent components:

* Subject/Observer = observer pattern optimized for embedded
* Services = low or zero overhead event-driven service subsystem
* Bit manipulation library for large datasets

## Platform-dependend components:

* LWiP Netbuf/PBUF wrapper into streambufs
* esp-idf implementations of aformentioned Services, such as for TWAI, GPTimer

Explicitly depends on estdlib
