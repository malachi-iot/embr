set(ROOT_DIR ..)

set(COMPONENT_ADD_INCLUDEDIRS 
    ${COMPONENT_ADD_INCLUDEDIRS}
    ${ROOT_DIR}/src
    ${ROOT_DIR}/ext/estdlib/src)

register_component()