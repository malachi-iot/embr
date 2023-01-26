cmake_minimum_required(VERSION 3.2)

project(embr VERSION 0.2.0 LANGUAGES CXX)

set(ROOT_DIR ..)
set(EXT_DIR ${ROOT_DIR}/ext)
set(ESTDLIB_DIR ${EXT_DIR}/estdlib/src)

if(NOT TARGET estd)
    add_subdirectory(${ESTDLIB_DIR} estd)
endif()

# NOTE: SOURCE_FILES not picked up ever.  Below target_include_directories
# INTERFACE effectively makes it a **/*.h header library.  So, source.cmake
# is effectively a NOOP and we keep it around just for reference.
include(sources.cmake)

#add_library(${PROJECT_NAME} ${SOURCE_FILES} ${INCLUDE_FILES})
# https://stackoverflow.com/questions/5957134/how-do-i-set-up-cmake-to-generate-header-only-projects
add_library(${PROJECT_NAME} INTERFACE)
add_library(malachi-iot::embr ALIAS ${PROJECT_NAME})

configure_file(../tools/cmake/version.in.h ${CMAKE_CURRENT_SOURCE_DIR}/embr/version.h)

# This line is largely superflous.  No linking happens for an INTERFACE library.
# estd itself does demand some linking.  I speculate this line will come in handy
# for consuming CMake projects which don't make explicit reference to estd
target_link_libraries(${PROJECT_NAME} INTERFACE estd)

#target_include_directories(${PROJECT_NAME} PUBLIC .)
target_include_directories(${PROJECT_NAME} INTERFACE .)

# Apparently interface libraries don't play well with LINKER_LANGUAGE.  Probably OK since
# a header library doesn't need its own link phase.
# For example: https://gitlab.kitware.com/cmake/cmake/-/issues/19528
# It seems only properties prefixed with INTERFACE_ are permitted
# (https://stackoverflow.com/questions/68502038/custom-properties-for-interface-libraries)
#set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

