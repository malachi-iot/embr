# EXPERIMENTAL
# chicken and egg, perhaps can't easily find this without specifying folder
# in the first place
#set(EMBR_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
#get_filename_component(EMBR_ROOT_DIR ${EMBR_ROOT_DIR} ABSOLUTE)
#set(ENV{EMBR_ROOT_DIR} ${EMBR_ROOT_DIR})

set(ROOT_DIR $ENV{EMBR_ROOT_DIR})

set(COMPONENT_ADD_INCLUDEDIRS 
    ${ROOT_DIR}/src
    ${ROOT_DIR}/ext/estdlib/src)

register_component()