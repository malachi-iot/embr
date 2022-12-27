# All references see README.md
cmake_minimum_required(VERSION 3.13)

# As per 5.1 this enables pico_w if someone isn't already specifying it in ENV
set(PICO_BOARD "pico_w")

include(../setenv.cmake)
include(wifi.cmake)

project(udp-echo LANGUAGES C CXX)

# Forcefully disable any FreeRTOS inclusion.  Not necessary for most
# people as the system usually auto deduces whether you want it.
set(FEATURE_ESTD_FREERTOS 0)


# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

set(LWIP_SOURCES
    ${ROOT_DIR}/test/rtos/esp-idf/udp-echo/main/udp-echo.cpp
    ${ROOT_DIR}/test/rtos/esp-idf/udp-echo/main/process.cpp
    )

add_subdirectory(../support support)

add_executable(${PROJECT_NAME}
    main.cpp
    ${LWIP_SOURCES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )

target_link_libraries(${PROJECT_NAME}
    #pico_cyw43_arch_lwip_threadsafe_background
    pico_cyw43_arch_lwip_poll
    pico_stdlib
    support embr estd)

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

