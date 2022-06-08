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
include_directories(${EMBR_DIR})
include_directories(${ESTD_DIR})

add_subdirectory(${EMBR_DIR} embr)
add_subdirectory(${ESTD_DIR} estd)

set(SOURCE_FILES
    main.cpp
    basics-test.cpp
    dataport-test.cpp
    datapump-test.cpp
    datapump-test.h
    netbuf-test.cpp
    observer-test.cpp
    experimental-test.cpp
    reader-test.cpp
    writer-test.cpp
    scheduler-test.cpp
    streambuf-test.cpp
    )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} embr)
