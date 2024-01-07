# This is the project CMakeLists.txt file for the test subproject 
cmake_minimum_required(VERSION 3.5)

SET(EMBR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../..)

add_compile_definitions("FEATURE_EMBR_ESP_TIMER_SCHEDULER=1")

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS "../components")

include(${EMBR_ROOT_DIR}/tools/esp-idf/project.cmake)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(timer-scheduler)