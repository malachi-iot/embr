# embr - Embedded Malachi Burke Resources

A monolithic library comprised of useful code for embedded oriented operations

## platform-independent components include:

* Subject/Observer = observer pattern optimized for embedded
* DataPump = I/O queueing
* DataPort = I/O queueing mated to Subject/Observer
* Netbuf = an abstraction of DMA/PBUF pattern

## platform-dependend components are planned to include:

* LWiP Netbuf/PBUF wrapper

Explicitly depends on estdlib
