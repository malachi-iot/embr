# This is the project CMakeLists.txt file for the test subproject 
cmake_minimum_required(VERSION 3.5)

SET(EMBR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../..)

# Include the components directory of the main application:
# Doing this explicit-style just as a marker how to do so
# rather than using the convenience project.cmake flavor
set(EXTRA_COMPONENT_DIRS "../components"
    ${EMBR_ROOT_DIR}/ext/estdlib/tools/esp-idf/components
    ${EMBR_ROOT_DIR}/tools/esp-idf/components
    )

# Remember, component compiler settings are cordoned off so we have to 
# expressly yank in version finder
include(${EMBR_ROOT_DIR}/ext/estdlib/tools/esp-idf/version_finder.cmake)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(udp-echo-dataport)