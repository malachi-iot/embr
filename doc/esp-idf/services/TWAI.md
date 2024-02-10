# embr::services TWAI

# Detail

# 1. Design Goals

TWAI service does not seek abstract away a transport mechanism.
Rather, gives `esp-idf` the service treatment, namely rebroadcasting alerts
via Subject-Observer pattern.  Also provided is a helper to spin
up a dedicated task to do this, though one may accomplish this via polling too.