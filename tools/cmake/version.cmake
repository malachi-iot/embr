# Depends on setvars

string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)

set(WORKING_DIR ${CMAKE_CURRENT_LIST_DIR}/in)

message(DEBUG "version.cmake: ${CMAKE_CURRENT_SOURCE_DIR} / ${WORKING_DIR}")

# DEBT: We can now directly use estd version.in.h
configure_file(
    ${WORKING_DIR}/version.in.h
    ${CMAKE_CURRENT_SOURCE_DIR}/embr/version.h)

# esp-idf: For internal testing
configure_file(
    ${WORKING_DIR}/idf_component.in.yml
    ${ROOT_DIR}/tools/esp-idf/components/embr/idf_component.yml)

# esp-idf: For general use
#configure_file(
#    ${WORKING_DIR}/idf_component.in.yml
#    ${ROOT_DIR}/idf_component.yml)

# For platformio
#configure_file(
#    ${WORKING_DIR}/library.in.json
#    ${ROOT_DIR}/library.json)
	