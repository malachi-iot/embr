cmake_minimum_required(VERSION 3.17)

project(embr-unit-test VERSION 1.0.0)

# Sorta works, but generates a billion warnings.  PGGCC-36 attempts to
# isolate why.  Disabing for that reason
option(EMBR_EXP_AUTOSTART "Experimental autostart testing" OFF)

# No longer dogfooding top level directly, since it makes CLion lose its mind
add_subdirectory(../../src embr)

CPMAddPackage("gh:catchorg/Catch2@3.11.0")
CPMAddPackage("gh:vector-of-bool/cmrc#952ffddba731fc110bd50409e8d2b8a06abbd237")

set(RESOURCES resources/playlist.json)

cmrc_add_resource_library(TESTRC ${RESOURCES} ALIAS testrc)

set(EXTRA_FILES
    README.md
    doc/exp/Retry.md
    doc/esp_helper/MenuConfig.md
    doc/JSON.md
    doc/Observer.md
)

list(TRANSFORM EXTRA_FILES PREPEND ${EMBR_ROOT_DIR}/)

set(SOURCE_FILES
    base64-test.cpp
    basics-test.cpp
    bits-test.cpp
    bitset-test.cpp
    ble-test.cpp
    breadcrumb-test.cpp
    debounce-test.cpp
    delegate-queue-test.cpp
    dsp-test.cpp
    flags-test.cpp
    json-test.cpp
    observer-test.cpp
    experimental-test.cpp
    mem/mem-v1-test.cpp
    mem/mem-v1-battery-test.cpp
    mem/mem-v1-func-test.cpp
    mem/mem-v1-ll-test.cpp
    objlist-test.cpp
    property-test.cpp
    retry-test.cpp
    runtime-ratio-test.cpp
    scheduler-test.cpp
    scheduler-v2-test.cpp
    service-test.cpp
    service-v2-test.cpp
    streambuf-test.cpp
    thunk-test.cpp
    units-test.cpp
    word-test.cpp
    )

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/sources.cmake)
    include(sources.cmake)
endif()

if(EMBR_EXP_AUTOSTART)
    target_compile_definitions(${PROJECT_NAME} PRIVATE FEATURE_EMBR_EXP_AUTOSTART=1)
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${EXTRA_FILES} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${estd_SOURCE_DIR}/estd/port/esp-idf/fake)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

target_compile_definitions(${PROJECT_NAME} PRIVATE UNIT_TESTING=1)
target_compile_definitions(${PROJECT_NAME} PRIVATE FEATURE_ESTD_OSTREAM_FLOAT=1)

target_link_libraries(${PROJECT_NAME} PRIVATE embr::embr)
target_link_libraries(${PROJECT_NAME} PRIVATE malachi-iot::estd)
target_link_libraries(${PROJECT_NAME} PRIVATE testrc Catch2::Catch2WithMain)
