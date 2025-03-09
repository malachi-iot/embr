# Not ready yet

cmake_minimum_required(VERSION 3.13)

project(embr-synthetic LANGUAGES CXX)

include(../../tools/cmake/setvars.cmake)
include(sources.cmake)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_11)
