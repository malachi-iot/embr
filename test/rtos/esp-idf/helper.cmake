SET(EMBR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/components")

include(${EMBR_ROOT_DIR}/tools/esp-idf/project.cmake)
