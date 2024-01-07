get_filename_component(EMBR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../.. ABSOLUTE)

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/components")

# Disables auto included components
set(COMPONENTS main)

add_compile_definitions("FEATURE_EMBR_ESP_LEGACY_DEBOUNCE=0")

include(${EMBR_ROOT_DIR}/tools/esp-idf/project.cmake)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
