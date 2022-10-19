cmake_minimum_required(VERSION 3.0)

project(embr-unit-test VERSION 1.0.0)

# necessary for Catch2
set(CMAKE_CXX_STANDARD 11)

add_definitions(-DUNIT_TESTING)

set(ROOT_DIR ../..)
set(EXT_DIR ${ROOT_DIR}/ext)
set(ESTD_DIR ${EXT_DIR}/estdlib/src)
set(CATCH_DIR ${EXT_DIR}/Catch2/single_include/catch2)
set(EMBR_DIR ${ROOT_DIR}/src)

include_directories(${CATCH_DIR})

add_subdirectory(${EMBR_DIR} embr)

set(SOURCE_FILES
    main.cpp
    basics-test.cpp
    bits-test.cpp
    dataport-test.cpp
    datapump-test.cpp
    datapump-test.h
    debounce-test.cpp
    observer-test.cpp
    experimental-test.cpp
    scheduler-test.cpp
    streambuf-test.cpp
    word-test.cpp
    )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} embr malachi-iot::estd)
