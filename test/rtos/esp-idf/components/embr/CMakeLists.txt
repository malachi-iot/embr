# Not yet working out too well since embr component is not visible to 'testable' component

set(ROOT_DIR ../../../../..)
#set(COMPONENT_SRCS "dummy.cpp")
set(COMPONENT_ADD_INCLUDEDIRS 
    ${ROOT_DIR}/src
    ${ROOT_DIR}/ext/estdlib/src)

register_component()