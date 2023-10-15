cmake_minimum_required(VERSION 3.1)

project(embr-unity-tests-lib VERSION 1.0.0)

set(ROOT_DIR ${PROJECT_SOURCE_DIR}/../..)
set(ESTDLIB_DIR ${ROOT_DIR}/ext/estdlib)
set(EMBR_DIR ${ROOT_DIR}/src)

# DEBT: Be advised this overwrites ROOT_DIR with 'estdlib' location
include(${ESTDLIB_DIR}/tools/cmake/setvars.cmake)

Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    Unity
    GIT_REPOSITORY  https://github.com/ThrowTheSwitch/Unity
    GIT_TAG         v2.5.2
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_MakeAvailable(Unity)

# add_subdirectory embr brings in estd for the ride, but we add in our own anyway just
# to test that the conditional add_subdirectory is behavior
# just
#add_subdirectory(${ESTDLIB_DIR}/src estd)
if(NOT TARGET embr)
    add_subdirectory(${EMBR_DIR} embr)
endif()

include(sources.cmake)

add_library(${PROJECT_NAME} 
    ${SOURCES}
    )

#target_link_libraries(embr-unity-tests-lib ALIAS unity-tests-lib)
add_library(unity-tests-lib ALIAS embr-unity-tests-lib)
target_link_libraries(${PROJECT_NAME} unity embr malachi-iot::estd)

target_include_directories(${PROJECT_NAME} PUBLIC .)
