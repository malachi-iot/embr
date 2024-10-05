# DEBT: Resolve this with existing source.cmake
get_filename_component(ROOT_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../..
    ABSOLUTE)

set(UNITY_DIR ${ROOT_DIR}/test/unity)

include(${UNITY_DIR}/sources.cmake)

list(TRANSFORM SOURCES PREPEND ${UNITY_DIR}/)

message(STATUS "ROOT: ${ROOT_DIR}")

set(SRCS_PATH ../../../../..)

set(COMPONENT_SRCS2 ${SOURCES}
    ${SRCS_PATH}/unity/delegate-queue.cpp
    ${SRCS_PATH}/unity/observer.cpp
    ${SRCS_PATH}/unity/pbuf.cpp
    ${SRCS_PATH}/unity/transport.cpp)

set(COMPONENT_SRCS
    ${COMPONENT_SRCS2}
    "dummy.cpp"
    "experimental.cpp"
    memory.cpp
    nvs.cpp
    power.cpp
    "timer.cpp")

set(COMPONENT_REQUIRES embr unity testable esp_helper esp_psram)

register_component()

component_compile_options("-Wstack-usage=512")

# Doesn't appear to make a difference
#component_compile_options("-Wstack-usage=128" "-DFEATURE_ESTD_CHARTRAITS=1")
