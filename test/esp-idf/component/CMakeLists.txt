cmake_minimum_required(VERSION 3.16)

include(../../../tools/cmake/setvars.cmake)

list(APPEND EXTRA_COMPONENT_DIRS ${ROOT_DIR}/tools/esp-idf/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(component)
