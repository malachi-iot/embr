cmake_minimum_required(VERSION 3.16)

if(ESP_PLATFORM)
    include(${CMAKE_CURRENT_LIST_DIR}/tools/esp-idf/component.cmake)
else()
    set(EMBR_BUILD_TESTS OFF CACHE BOOL "Build embr tests")

    project(embr-meta)

    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src embr)

    if(EMBR_BUILD_TESTS OR PROJECT_IS_TOP_LEVEL)
        add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test/catch catch)
    endif()
endif()
