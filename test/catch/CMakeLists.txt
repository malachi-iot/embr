cmake_minimum_required(VERSION 3.0)

project(embr-unit-test VERSION 1.0.0)

# necessary for Catch2
set(CMAKE_CXX_STANDARD 11)

add_definitions(-DUNIT_TESTING)

include(../../tools/cmake/setvars.cmake)

add_subdirectory(${EXT_DIR}/estdlib/ext/Catch2 catch)
add_subdirectory(${ROOT_DIR}/src embr)

set(SOURCE_FILES
    main.cpp
    basics-test.cpp
    bits-test.cpp
    debounce-test.cpp
    observer-test.cpp
    experimental-test.cpp
    property-test.cpp
    runtime-ratio-test.cpp
    scheduler-test.cpp
    service-test.cpp
    streambuf-test.cpp
    word-test.cpp
    )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} embr malachi-iot::estd Catch2)
