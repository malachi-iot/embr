# This is the project CMakeLists.txt file for the test subproject 
cmake_minimum_required(VERSION 3.5)

# FIX: Should probably be default when move semantic is available
add_definitions(-DFEATURE_EMBR_DATAPUMP_INLINE)

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS "../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(udp-echo-dataport)