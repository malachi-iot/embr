# All references see README.md
cmake_minimum_required(VERSION 3.13)

# As per 5.1 this enables pico_w if someone isn't already specifying it in ENV
set(PICO_BOARD "pico_w")

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../..)

#include(../setenv.cmake)
#include(wifi.cmake)

project(embr-unity LANGUAGES C CXX)

# Forcefully disable any FreeRTOS inclusion.  Not necessary for most
# people as the system usually auto deduces whether you want it.
set(FEATURE_ESTD_FREERTOS 0)


# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

message(DEBUG "unity: board=${PICO_BOARD}")
# DEBT: I'd much prefer to merely elevate/reuse existing PICO_BOARD definition, but:
# a) it's not available to unit-tests lib, presumably because of SDK's interface behavior
# b) if I name this PICO_BOARD, we get duplicate definitions when compiling actual SDK
add_compile_definitions(EMBR_PICO_BOARD=${PICO_BOARD})
if(PICO_BOARD STREQUAL pico_w)
add_compile_definitions(EMBR_PICOW_BOARD=${PICO_BOARD})
endif()

add_subdirectory(${ROOT_DIR}/test/unity unit-tests)

# DEBT: This sneaks in our esp_log.h shim - viable, but confusing
target_include_directories(unity-tests-lib PRIVATE ../udp-echo)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    unity-tests-lib unity
    embr estd)

if(PICO_BOARD STREQUAL pico_w)
    target_link_libraries(${PROJECT_NAME}
        #pico_cyw43_arch_lwip_threadsafe_background
        pico_cyw43_arch_lwip_poll
        )
    target_link_libraries(unity-tests-lib
        #pico_cyw43_arch_lwip_threadsafe_background
        pico_cyw43_arch_lwip_poll
        )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE .)

# enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# wait indefinitely for a CDC to appear so we can see test results
# Beware -- reports indicate this hangs UART indefinitely
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=-1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})

