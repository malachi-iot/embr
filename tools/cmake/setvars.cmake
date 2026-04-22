include_guard(GLOBAL)

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
set(EMBR_ROOT_DIR ${ROOT_DIR} CACHE STRING "Absolute path to embr")
#set(EMBR_ROOT_DIR ${ROOT_DIR})

include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)
