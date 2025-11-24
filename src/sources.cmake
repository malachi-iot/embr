set(INCLUDE_FILES
    embr/detail/debounce.h
    embr/detail/debounce.hpp

    embr/observer.h

    embr/scheduler.h

    embr/streambuf.h
    embr/transport-descriptor.h

    embr/exp/netbuf-alloc.h
    embr/exp/pbuf.h embr/exp/retry-v2.h embr/exp/dataport-v2.h
    embr/exp/thunk.h

    embr/storage/funclist.h
    embr/storage/fwd.h
    embr/storage/objlist.h
    )

set(SOURCE_FILES
    base64.cpp
    dsp.cpp
    embr/internal/general.cpp
    embr/service/service_v1.cpp
    embr/service/service_v2.cpp
    )

set(ESP_IDF_SOURCE_FILES ${SOURCE_FILES}
    embr/platform/esp-idf/debounce.cpp
    embr/platform/esp-idf/service.cpp
    embr/platform/esp-idf/timer-scheduler.cpp
    )

