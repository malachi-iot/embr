cmake_minimum_required(VERSION 2.8)

project(embr-netbuf-lib)

set(ROOT_DIR ..)
set(EXT_DIR ${ROOT_DIR}/ext)
set(ESTDLIB_DIR ${EXT_DIR}/estdlib/src)

include_directories(${ESTDLIB_DIR})

add_subdirectory(${ESTDLIB_DIR} estdlib)

set(SOURCE_FILES
    main.cpp
    embr/netbuf.h
    )

add_library(${PROJECT_NAME} ${SOURCE_FILES})
