cmake_minimum_required(VERSION 3.13)

project(embr VERSION 0.4.0 LANGUAGES CXX
    DESCRIPTION "Embedded utility library"
    HOMEPAGE_URL "https://github.com/malachi-iot/embr"
    )

include(../tools/cmake/setvars.cmake)

# NOTE: SOURCE_FILES not picked up ever.  Below target_include_directories
# INTERFACE effectively makes it a **/*.h header library.  So, source.cmake
# is effectively a NOOP and we keep it around just for reference.
include(sources.cmake)
include(${ROOT_DIR}/tools/cmake/version.cmake)

# https://stackoverflow.com/questions/5957134/how-do-i-set-up-cmake-to-generate-header-only-projects
# TODO: Bring this guy in
add_library(${PROJECT_NAME} INTERFACE)
add_library(malachi-iot::embr ALIAS ${PROJECT_NAME})
add_library(embr_static STATIC ${SOURCE_FILES})
add_library(embr::embr ALIAS embr_static)
add_library(embr::embr_interface ALIAS ${PROJECT_NAME})

CPMAddPackage("gh:malachi-iot/estdlib@0.8.5")

# DEBT: Was done for macOS, but it doesn't pick it up.  Once that is resolved,
# bring these in
#target_compile_features(estd INTERFACE cxx_std_11)
#target_compile_features(embr INTERFACE cxx_std_11)

target_sources(${PROJECT_NAME} INTERFACE ${SOURCE_FILES} ${INCLUDE_FILES})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_11)

# This line is largely superflous.  No linking happens for an INTERFACE library.
# estd itself does demand some linking.  I speculate this line will come in handy
# for consuming CMake projects which don't make explicit reference to estd
target_link_libraries(${PROJECT_NAME} INTERFACE estd)
target_link_libraries(embr_static PUBLIC estd)

#target_include_directories(${PROJECT_NAME} PUBLIC .)
target_include_directories(${PROJECT_NAME} INTERFACE .)
target_include_directories(embr_static PUBLIC .)

# Apparently interface libraries don't play well with LINKER_LANGUAGE.  Probably OK since
# a header library doesn't need its own link phase.
# For example: https://gitlab.kitware.com/cmake/cmake/-/issues/19528
# It seems only properties prefixed with INTERFACE_ are permitted
# (https://stackoverflow.com/questions/68502038/custom-properties-for-interface-libraries)
#set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

