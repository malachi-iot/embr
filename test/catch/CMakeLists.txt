cmake_minimum_required(VERSION 2.8)

project(embr-unit-test)

# necessary for Catch2
set(CMAKE_CXX_STANDARD 11)

set(ROOT_DIR ../..)
set(EXT_DIR ${ROOT_DIR}/ext)
set(ESTDLIB_DIR ${EXT_DIR}/estdlib/src)
set(CATCH_DIR ${EXT_DIR}/Catch2/single_include/catch2)
set(EMBR_NETBUF_DIR ${ROOT_DIR}/src)

include_directories(${CATCH_DIR})
include_directories(${EMBR_NETBUF_DIR})
include_directories(${ESTDLIB_DIR})

add_subdirectory(${EMBR_NETBUF_DIR} netbuf)

set(SOURCE_FILES
    main.cpp
    basics-test.cpp
    experimental-test.cpp
    reader-test.cpp
    writer-test.cpp
    )

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(embr-netbuf-lib)
