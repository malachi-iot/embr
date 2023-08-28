set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(SRC_DIR ${ROOT_DIR}/src)

set(COMPONENT_REQUIRES estdlib driver esp_event esp_timer)

# Get ESP_IDF_SOURCE_FILES from here
include(${SRC_DIR}/sources.cmake)

list(TRANSFORM ESP_IDF_SOURCE_FILES PREPEND ${SRC_DIR}/)

set(COMPONENT_SRCS ${ESP_IDF_SOURCE_FILES})
set(COMPONENT_ADD_INCLUDEDIRS ${SRC_DIR})

register_component()