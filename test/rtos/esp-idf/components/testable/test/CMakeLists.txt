set(COMPONENT_SRCS
    "dummy.cpp"
    "pbuf.cpp"
    "transport.cpp"
    "experimental.cpp"
    "dataport.cpp")

set(COMPONENT_REQUIRES estdlib embr unity testable esp_helper)

register_component()

component_compile_options("-Wstack-usage=128")

# Doesn't appear to make a difference
#component_compile_options("-Wstack-usage=128" "-DFEATURE_ESTD_CHARTRAITS=1")
