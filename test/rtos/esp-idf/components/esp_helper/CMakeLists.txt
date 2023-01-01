set(COMPONENT_SRCS "esp-helper.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES nvs_flash embr estdlib esp_event esp_wifi)

# 31DEC22 MB DEBT: Brute forcing this in, days are numbered for our own
# ESP versioning though.  Only tested on esp-idf v5
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../..)
get_filename_component(ROOT_DIR ${ROOT_DIR} ABSOLUTE)
include(${ROOT_DIR}/ext/estdlib/tools/esp-idf/version_finder.cmake)

register_component()