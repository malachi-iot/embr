set(COMPONENT_SRCS "esp-helper.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

# As we transition from 'estdlib' to 'estd' naming
if(NOT DEFINED ESTD_COMPONENT_NAME)
	set(ESTD_COMPONENT_NAME estdlib)
endif()

# DEBT: Phase out esp_event here, service::EventLoop supersedes it
# DEBT: Phase out estd here, since embr is supposed to carry that forward... maybe.. since it's not PRIV_REQUIRES
set(COMPONENT_REQUIRES nvs_flash embr ${ESTD_COMPONENT_NAME} esp_event esp_wifi)

# 25AUG23 MB: Not sure how to conditionally include led_strip component
# itself, and I don't want to ALWAYS include it.  That being a prerequisite,
# this remains commented out
#if(CONFIG_BOARD_ESP32C3_DEVKITM_1)
#    list(APPEND COMPONENT_SRCS led_strip.cpp)
#endif()

# 31DEC22 MB DEBT: Brute forcing this in, days are numbered for our own
# ESP versioning though.  Only tested on esp-idf v5
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../..)
get_filename_component(ROOT_DIR ${ROOT_DIR} ABSOLUTE)
include(${ROOT_DIR}/ext/estdlib/tools/esp-idf/version_finder.cmake)

register_component()
