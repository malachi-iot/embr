SET(EMBR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

include(${EMBR_ROOT_DIR}/ext/estdlib/tools/esp-idf/project.cmake)

set(EXTRA_COMPONENT_DIRS
    "${EXTRA_COMPONENT_DIRS}"
    "${CMAKE_CURRENT_LIST_DIR}/components"
    )
