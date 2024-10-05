cmake_minimum_required(VERSION 3.15)

project(embr-unity-lib VERSION 1.0.0)

include(${CMAKE_CURRENT_LIST_DIR}/../../tools/cmake/setvars.cmake)

set(EMBR_DIR ${ROOT_DIR}/src)

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

# As per https://stackoverflow.com/questions/25199677/how-to-detect-if-current-scope-has-a-parent-in-cmake

get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
    message(STATUS "${PROJECT_NAME}: Building as library")
    add_library(${PROJECT_NAME} ${SOURCES})

    add_library(unity-tests-lib ALIAS ${PROJECT_NAME})
else()
    message(STATUS "${PROJECT_NAME}: Building as standalone")
    add_executable(${PROJECT_NAME} main.cpp ${SOURCES})
endif()

target_link_libraries(${PROJECT_NAME} unity embr malachi-iot::estd)

target_include_directories(${PROJECT_NAME} PUBLIC .)
