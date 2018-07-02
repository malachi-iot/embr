cmake_minimum_required(VERSION 2.8)

project(embr-unit-test)

# necessary for Catch2
set(CMAKE_CXX_STANDARD 11)

set(ROOT_DIR ../..)
set(EXT_DIR ${ROOT_DIR}/ext)
#set(ESTDLIB_DIR ${EXIT_DIR}/estdlib/src)
set(CATCH_DIR ${EXT_DIR}/Catch2/include)
set(EMBR_NETBUF_DIR ${ROOT_DIR}/src)

include_directories(${CATCH_DIR})

add_subdirectory(${EMBR_NETBUF_DIR} netbuf)

set(SOURCE_FILES
    main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(embr-netbuf-lib)
