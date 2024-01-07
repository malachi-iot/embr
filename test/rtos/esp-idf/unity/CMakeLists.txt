# This is the project CMakeLists.txt file for the test subproject 
cmake_minimum_required(VERSION 3.5)

SET(EMBR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../..)

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS "../components")

include(${EMBR_ROOT_DIR}/tools/esp-idf/project.cmake)

# Set the components to include the tests for.
# This can be overriden from CMake cache:
# - when invoking CMake directly: cmake -D TEST_COMPONENTS="xxxxx" ..
# - when using idf.py: idf.py build -T xxxxx
#
set(TEST_COMPONENTS "testable" CACHE STRING "List of components to test")

# Turns on legacy (warning generating) esp-idf 4.x timer wrapper
#add_compile_definitions("FEATURE_EMBR_ESP_TIMER_SCHEDULER=1")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(unit_test_test)