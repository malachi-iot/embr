# DEBT: Migrate in setenv

# As per 5.1 this enables pico_w if someone isn't already specifying it in ENV
set(PICO_BOARD "pico_w")

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
get_filename_component(ROOT_DIR ${ROOT_DIR} ABSOLUTE)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# picks up estd for us also
add_subdirectory(${ROOT_DIR}/src embr)
