cmake_minimum_required(VERSION 3.17)

project(embr-unit-test VERSION 1.0.0)

# Sorta works, but generates a billion warnings.  PGGCC-36 attempts to
# isolate why.  Disabing for that reason
option(EMBR_EXP_AUTOSTART "Experimental autostart testing" OFF)

add_definitions(-DUNIT_TESTING=1)

include(../../tools/cmake/setvars.cmake)

add_subdirectory(${ROOT_DIR}/src embr)

Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
    GIT_TAG         v2.13.10
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_MakeAvailable(Catch2)

set(SOURCE_FILES
    main.cpp
    basics-test.cpp
    bits-test.cpp
    bitset-test.cpp
    debounce-test.cpp
    delegate-queue-test.cpp
    json-test.cpp
    observer-test.cpp
    experimental-test.cpp
    property-test.cpp
    runtime-ratio-test.cpp
    scheduler-test.cpp
    service-test.cpp
    streambuf-test.cpp
    word-test.cpp
    )

if(EMBR_EXP_AUTOSTART)
    target_compile_definitions(${PROJECT_NAME} PRIVATE FEATURE_EMBR_EXP_AUTOSTART=1)
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# necessary for Catch2
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

target_compile_definitions(${PROJECT_NAME} PRIVATE UNIT_TESTING=1)

target_link_libraries(${PROJECT_NAME} embr malachi-iot::estd Catch2)
