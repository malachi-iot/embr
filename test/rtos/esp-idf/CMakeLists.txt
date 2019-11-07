# This cmake doesn't really target anything.  Not sure what
# "unit_test" vs "unit_test_test" usefulness is, since source
# example put proper unit tests into "unit_test_test"
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(unit_test)
