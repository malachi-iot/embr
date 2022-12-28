cmake_minimum_required(VERSION 3.13)

# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(embr-support LANGUAGES C CXX)

include(../setup.cmake)

set(EMBR_ROOT_DIR ${ROOT_DIR} PARENT_SCOPE)
set(PICO_CYW43_ARCH_LIB ${PICO_CYW43_ARCH_LIB} PARENT_SCOPE)

set(SOURCE_FILES init.cpp loop.cpp)
add_library(${PROJECT_NAME} STATIC)
add_library(support ALIAS ${PROJECT_NAME})
add_library(embr::support ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})

# Gets esp_log.h wrapper - might be better as a tiny interface library instead though. hmm
target_include_directories(${PROJECT_NAME} PUBLIC ${ROOT_DIR}/ext/estdlib/src/estd/port/esp-idf/fake)
target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    estd embr
    ${PICO_CYW43_ARCH_LIB}
    )

# DEBT: Unsure how this meshes with app also setting this variable
pico_enable_stdio_usb(${PROJECT_NAME} 1)


