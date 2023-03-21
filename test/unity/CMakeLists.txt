cmake_minimum_required(VERSION 3.1)

project(unity-tests-lib)

set(ROOT_DIR ${PROJECT_SOURCE_DIR}/../..)
set(ESTDLIB_DIR ${ROOT_DIR}/ext/estdlib)
set(EMBR_DIR ${ROOT_DIR}/src)
set(UNITY_DIR ${ESTDLIB_DIR}/ext/Unity)

# add_subdirectory embr brings in estd for the ride, but we add in our own anyway just
# to test that the conditional add_subdirectory is behavior
# just
#add_subdirectory(${ESTDLIB_DIR}/src estd)
if(NOT TARGET embr)
    add_subdirectory(${EMBR_DIR} embr)
endif()
add_subdirectory(${UNITY_DIR} unity)

include(sources.cmake)

add_library(${PROJECT_NAME} 
    ${SOURCES}
    )

target_link_libraries(${PROJECT_NAME} unity embr malachi-iot::estd)
target_link_libraries(${PROJECT_NAME} embr::support)

target_include_directories(${PROJECT_NAME} PUBLIC .)
