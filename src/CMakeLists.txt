cmake_minimum_required(VERSION 3.13)

project(embr VERSION 0.4.7 LANGUAGES CXX
    DESCRIPTION "Embedded utility library"
    HOMEPAGE_URL "https://github.com/malachi-iot/embr"
    )

include(../tools/cmake/setvars.cmake)
include(../tools/cmake/options.cmake)

set(INTERFACE_NAME ${PROJECT_NAME}-interface)

# Grab SOURCE_FILES
include(sources.cmake)
include(${ROOT_DIR}/tools/cmake/version.cmake)

add_library(${INTERFACE_NAME} INTERFACE)
add_library(${PROJECT_NAME}-static STATIC)
# 30MAY26 MB - SHARED flavor not tested
add_library(${PROJECT_NAME}-shared SHARED)
add_library(malachi-iot::embr ALIAS ${INTERFACE_NAME})
add_library(embr::embr ALIAS ${PROJECT_NAME}-static)
add_library(embr::interface ALIAS ${INTERFACE_NAME})

CPMAddPackage("gh:malachi-iot/estdlib@0.8.12-beta1")
#CPMAddPackage("gh:malachi-iot/estdlib#8e5875925a4846a84b281615e8a6000522469687")

# DEBT: Was done for macOS, but it doesn't pick it up.  Once that is resolved,
# bring these in
#target_compile_features(estd INTERFACE cxx_std_11)
#target_compile_features(embr INTERFACE cxx_std_11)

target_sources(${INTERFACE_NAME} INTERFACE ${SOURCE_FILES} ${INCLUDE_FILES})

target_compile_features(${INTERFACE_NAME} INTERFACE cxx_std_11)

# More peacemeal about setting up linking/includes because WHOLE_ARCHIVE feature
# starts duplicating symbols otherwise
target_link_libraries(${INTERFACE_NAME} INTERFACE estd)
target_link_libraries(${PROJECT_NAME}-static PRIVATE embr::interface)
target_link_libraries(${PROJECT_NAME}-shared PRIVATE embr::interface)

target_include_directories(${INTERFACE_NAME} INTERFACE .)
target_include_directories(${PROJECT_NAME}-static PUBLIC .)
target_include_directories(${PROJECT_NAME}-shared PUBLIC .)

# Apparently interface libraries don't play well with LINKER_LANGUAGE.  Probably OK since
# a header library doesn't need its own link phase.
# For example: https://gitlab.kitware.com/cmake/cmake/-/issues/19528
# It seems only properties prefixed with INTERFACE_ are permitted
# (https://stackoverflow.com/questions/68502038/custom-properties-for-interface-libraries)
#set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

