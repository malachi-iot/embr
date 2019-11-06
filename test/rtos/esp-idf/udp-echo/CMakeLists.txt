# This is the project CMakeLists.txt file for the test subproject 
cmake_minimum_required(VERSION 3.5)

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS "../components")

set(EMBR_ROOT_DIR ../../../..)
get_filename_component(EMBR_ROOT_DIR ${EMBR_ROOT_DIR} ABSOLUTE)
set(ENV{EMBR_ROOT_DIR} ${EMBR_ROOT_DIR})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(udp-echo)