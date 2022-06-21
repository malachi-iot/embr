cmake_minimum_required(VERSION 3.2)

project(embr VERSION 0.1.2 LANGUAGES CXX)

set(ROOT_DIR ..)
set(EXT_DIR ${ROOT_DIR}/ext)
set(ESTDLIB_DIR ${EXT_DIR}/estdlib/src)

include_directories(${ESTDLIB_DIR})

# libs don't add subdirectories themselves, since it collides
# with an add_subdirectory that a consuming app may issue
#add_subdirectory(${ESTDLIB_DIR} estdlib)

set(SOURCE_FILES
    embr/dataport.h
    embr/dataport.hpp
    embr/datapump.h
    embr/datapump.hpp
    embr/events.h

    embr/exp/netbuf-alloc.h

    embr/netbuf.h
    embr/netbuf-static.h
    embr/netbuf-dynamic.h
    embr/netbuf-reader.h
    embr/netbuf-writer.h

    embr/observer.h

    embr/scheduler.h

    embr/streambuf.h
    embr/transport-descriptor.h
        embr/exp/datapump-v2.h embr/exp/pbuf.h embr/exp/retry-v2.h embr/exp/datapump-core-v2.h embr/exp/dataport-v2.h)

#add_library(${PROJECT_NAME} ${SOURCE_FILES})
# https://stackoverflow.com/questions/5957134/how-do-i-set-up-cmake-to-generate-header-only-projects
add_library(${PROJECT_NAME} INTERFACE)
add_library(malachi-iot::embr ALIAS ${PROJECT_NAME})
# 'embr-lib' name is now deprecated
add_library(embr-lib ALIAS ${PROJECT_NAME})

#target_include_directories(${PROJECT_NAME} PUBLIC .)
target_include_directories(${PROJECT_NAME} INTERFACE .)

# Apparently interface libraries don't play well with LINKER_LANGUAGE.  Probably OK since
# a header library doesn't need its own link phase.
# For example: https://gitlab.kitware.com/cmake/cmake/-/issues/19528
# It seems only properties prefixed with INTERFACE_ are permitted
# (https://stackoverflow.com/questions/68502038/custom-properties-for-interface-libraries)
#set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

