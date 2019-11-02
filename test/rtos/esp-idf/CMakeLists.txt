# This cmake doesn't really target anything.  Not sure what
# "unit_test" vs "unit_test_test" usefulness is, since source
# example put proper unit tests into "unit_test_test"
cmake_minimum_required(VERSION 3.5)

# just pisses it off.  I think we may be picking up 'unity' folder
# too and that breaks things
set(ENV{EMBR_ROOT_DIR} ../../../..)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(unit_test)
