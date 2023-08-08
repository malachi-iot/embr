# This is the project CMakeLists.txt file for the test subproject 
cmake_minimum_required(VERSION 3.5)

add_compile_definitions("FEATURE_EMBR_ESP_LEGACY_DEBOUNCE=0")
add_compile_definitions("FEATURE_EMBR_ESP_TIMER_SCHEDULER=0")

include(../../setvars.cmake)

project(twai-service)
