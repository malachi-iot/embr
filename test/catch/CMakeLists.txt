cmake_minimum_required(VERSION 3.0)

project(embr-unit-test VERSION 1.0.0)

# necessary for Catch2
set(CMAKE_CXX_STANDARD 11)

add_definitions(-DUNIT_TESTING)

set(ROOT_DIR ../..)
set(EXT_DIR ${ROOT_DIR}/ext)
set(ESTD_DIR ${EXT_DIR}/estdlib/src)
set(EMBR_DIR ${ROOT_DIR}/src)

include_directories(${CATCH_DIR})

add_subdirectory(${EXT_DIR}/estdlib/ext/Catch2 catch)
add_subdirectory(${EMBR_DIR} embr)

set(SOURCE_FILES
    main.cpp
    basics-test.cpp
    bits-test.cpp
    debounce-test.cpp
    observer-test.cpp
    experimental-test.cpp
    runtime-ratio-test.cpp
    scheduler-test.cpp
    streambuf-test.cpp
    word-test.cpp
    )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} embr malachi-iot::estd Catch2)
