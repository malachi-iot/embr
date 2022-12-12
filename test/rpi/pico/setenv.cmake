# initialize the SDK directly
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)

add_subdirectory(${ROOT_DIR}/src embr)

