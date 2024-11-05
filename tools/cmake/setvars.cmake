include_guard(GLOBAL)

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
set(EMBR_ROOT_DIR ${ROOT_DIR})
set(EXT_DIR ${ROOT_DIR}/ext)
message(DEBUG "embr: EXT_DIR=${EXT_DIR}")

set(ESTD_DIR ${EXT_DIR}/estdlib/src)

include(${EXT_DIR}/estdlib/tools/cmake/fetchcontent.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)
