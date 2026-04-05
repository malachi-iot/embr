cmake_minimum_required(VERSION 3.13)

project(embr VERSION 0.4.4 LANGUAGES CXX
    DESCRIPTION "Embedded utility library"
    HOMEPAGE_URL "https://github.com/malachi-iot/embr"
    )

include(../tools/cmake/setvars.cmake)

set(INTERFACE_NAME ${PROJECT_NAME}-interface)

# Grab SOURCE_FILES
include(sources.cmake)
include(${ROOT_DIR}/tools/cmake/version.cmake)

add_library(${INTERFACE_NAME} INTERFACE)
add_library(${PROJECT_NAME}-static STATIC)
add_library(malachi-iot::embr ALIAS ${INTERFACE_NAME})
add_library(embr::embr ALIAS ${PROJECT_NAME}-static)
add_library(embr::interface ALIAS ${INTERFACE_NAME})

#CPMAddPackage("gh:malachi-iot/estdlib@0.8.11-beta3")
CPMAddPackage("gh:malachi-iot/estdlib#1ac09d8505d0eb04a3f9ea00cb794f31eaf0d14b")

# DEBT: Was done for macOS, but it doesn't pick it up.  Once that is resolved,
# bring these in
#target_compile_features(estd INTERFACE cxx_std_11)
#target_compile_features(embr INTERFACE cxx_std_11)

target_sources(${INTERFACE_NAME} INTERFACE ${SOURCE_FILES} ${INCLUDE_FILES})

target_compile_features(${INTERFACE_NAME} INTERFACE cxx_std_11)

target_link_libraries(${INTERFACE_NAME} INTERFACE estd)
target_link_libraries(${PROJECT_NAME}-static PUBLIC embr::interface)

target_include_directories(${INTERFACE_NAME} INTERFACE .)

# Apparently interface libraries don't play well with LINKER_LANGUAGE.  Probably OK since
# a header library doesn't need its own link phase.
# For example: https://gitlab.kitware.com/cmake/cmake/-/issues/19528
# It seems only properties prefixed with INTERFACE_ are permitted
# (https://stackoverflow.com/questions/68502038/custom-properties-for-interface-libraries)
#set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

