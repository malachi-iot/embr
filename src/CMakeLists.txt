cmake_minimum_required(VERSION 3.13)

project(embr VERSION 0.3.1 LANGUAGES CXX)

include(../tools/cmake/setvars.cmake)
include(../tools/cmake/version.cmake)

if(NOT TARGET estd)
    add_subdirectory(${ESTD_DIR} estd)
endif()

# NOTE: SOURCE_FILES not picked up ever.  Below target_include_directories
# INTERFACE effectively makes it a **/*.h header library.  So, source.cmake
# is effectively a NOOP and we keep it around just for reference.
include(sources.cmake)

# https://stackoverflow.com/questions/5957134/how-do-i-set-up-cmake-to-generate-header-only-projects
add_library(${PROJECT_NAME} INTERFACE)
add_library(malachi-iot::embr ALIAS ${PROJECT_NAME})

# DEBT: Was done for macOS, but it doesn't pick it up.  Once that is resolved,
# bring these in
#target_compile_features(estd INTERFACE cxx_std_11)
#target_compile_features(embr INTERFACE cxx_std_11)

target_sources(${PROJECT_NAME} INTERFACE ${SOURCE_FILES} ${INCLUDE_FILES})

configure_version(${ROOT_DIR} ${CMAKE_CURRENT_LIST_DIR}/embr/)

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

