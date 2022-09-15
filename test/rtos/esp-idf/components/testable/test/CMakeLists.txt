# DEBT: Resolve this with existing source.cmake
set(SRCS_PATH ../../../../..)
set(COMPONENT_SRCS2 
    ${SRCS_PATH}/unity/bits.cpp
    ${SRCS_PATH}/unity/lwip.cpp
    ${SRCS_PATH}/unity/word.cpp)

set(COMPONENT_SRCS
    ${COMPONENT_SRCS2}
    "dummy.cpp"
    "pbuf.cpp"
    "transport.cpp"
    "experimental.cpp"
    "dataport.cpp")

set(COMPONENT_REQUIRES estdlib embr unity testable esp_helper)

register_component()

component_compile_options("-Wstack-usage=512")

# Doesn't appear to make a difference
#component_compile_options("-Wstack-usage=128" "-DFEATURE_ESTD_CHARTRAITS=1")
