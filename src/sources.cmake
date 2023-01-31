set(INCLUDE_FILES
    embr/detail/debounce.h
    embr/detail/debounce.hpp

    embr/observer.h

    embr/scheduler.h

    embr/streambuf.h
    embr/transport-descriptor.h

    embr/exp/netbuf-alloc.h
    embr/exp/pbuf.h embr/exp/retry-v2.h embr/exp/dataport-v2.h
    )

# NOTE: Really, most of embr requires c++11 onward

if(NOT CMAKE_CXX_STANDARD EQUAL 98)
    set(SOURCE_FILES
        embr/service/service_v1.cpp
        )
endif()

set(ESP_IDF_SOURCE_FILES ${SOURCE_FILES}
    embr/platform/esp-idf/debounce.cpp
    embr/platform/esp-idf/timer-scheduler.cpp
    )

