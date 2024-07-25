# DEBT: Copy/pasted in from estdlib - make this into a function that
# estdlib eventually defines so that we can share code

function(configure_version ROOT_DIR VERSION_DIR)
    set(WORKING_DIR ${ROOT_DIR}/tools/cmake/in)

    message(STATUS "version.cmake: ${CMAKE_CURRENT_SOURCE_DIR} / ${WORKING_DIR}")
    
    # string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)

    configure_file(
        ${WORKING_DIR}/version.in.h
        ${VERSION_DIR}/version.h)

    # esp-idf: For internal testing
    #configure_file(
    #    ${WORKING_DIR}/idf_component.in.yml
    #    ${ROOT_DIR}/tools/esp-idf/components/estdlib/idf_component.yml)

    # esp-idf: For general use
    configure_file(
        ${WORKING_DIR}/idf_component.in.yml
        ${ROOT_DIR}/idf_component.yml)

    # For platformio
    configure_file(
        ${WORKING_DIR}/library.in.json
        ${ROOT_DIR}/library.json)
        
endfunction()

