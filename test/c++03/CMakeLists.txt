cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 98)

# DEBT: This is not always true for C++98 scenarios, but implicitly true for ADSPBFIN (inttypes.h isn't
# available, but it seems to have those defs) and most other compilers I use
add_compile_definitions(FEATURE_STD_INTTYPES)

project(embr-unity-tests VERSION 1.0)

set(UNITY_TESTS_DIR ../unity)

add_subdirectory(${UNITY_TESTS_DIR} unity-tests-lib)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} unity-tests-lib)