# As per 5.1 this enables pico_w if someone isn't already specifying it in ENV
set(PICO_BOARD "pico_w")

include(${CMAKE_CURRENT_LIST_DIR}/../../../tools/cmake/setvars.cmake)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# picks up estd for us also
if(NOT TARGET embr)
    add_subdirectory(${ROOT_DIR}/src embr)
endif()

# DEBT: Document this feature flag
if(NOT DEFINED FEATURE_EMBR_TEST_RPI_PICO_W_POLL)
    set(FEATURE_EMBR_TEST_RPI_PICO_W_POLL 1)
endif()

# DEBT: I have a feeling there might be an rpi pico sdk 
# official mechanism for this
if(FEATURE_EMBR_TEST_RPI_PICO_W_POLL)
    set(PICO_CYW43_ARCH_LIB pico_cyw43_arch_lwip_poll)
else()
    set(PICO_CYW43_ARCH_LIB pico_cyw43_arch_lwip_threadsafe_background)
endif()
