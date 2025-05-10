cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(embr-support LANGUAGES C CXX ASM)

include(../setup.cmake)

set(PICO_CYW43_ARCH_LIB ${PICO_CYW43_ARCH_LIB} PARENT_SCOPE)

set(SOURCE_FILES init.cpp loop.cpp)
add_library(${PROJECT_NAME} STATIC)
add_library(support ALIAS ${PROJECT_NAME})
add_library(embr::support ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})

message(DEBUG "PICO_CYW43_ARCH_LIB: ${PICO_CYW43_ARCH_LIB}")
message(DEBUG "estd at: ${estd_SOURCE_DIR}")

# Gets esp_log.h wrapper - might be better as a tiny interface library instead though. hmm
target_include_directories(${PROJECT_NAME} PUBLIC ${estd_SOURCE_DIR}/estd/port/esp-idf/fake)
target_include_directories(${PROJECT_NAME} PUBLIC .)

get_target_property(includes ${PROJECT_NAME} INCLUDE_DIRECTORIES)
message(STATUS "Include directories for ${PROJECT_NAME}: ${includes}")


target_link_libraries(${PROJECT_NAME}
    PUBLIC
    pico_cyw43_arch_none
    pico_cyw43_driver
    pico_stdlib
    estd embr
    ${PICO_CYW43_ARCH_LIB}
    )

# DEBT: Unsure how this meshes with app also setting this variable
pico_enable_stdio_usb(${PROJECT_NAME} 1)


