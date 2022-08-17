cmake_minimum_required(VERSION 3.1)

project(unity-tests-lib)

set(ROOT_DIR ${PROJECT_SOURCE_DIR}/../..)
set(ESTDLIB_DIR ${ROOT_DIR}/ext/estdlib)
set(EMBR_DIR ${ROOT_DIR}/src)
set(UNITY_DIR ${ESTDLIB_DIR}/ext/Unity)

add_subdirectory(${EMBR_DIR} embr)
add_subdirectory(${UNITY_DIR} unity)

include(sources.cmake)

add_library(${PROJECT_NAME} 
    ${SOURCES}
    )

target_link_libraries(${PROJECT_NAME} unity embr)

target_include_directories(${PROJECT_NAME} PUBLIC .)
