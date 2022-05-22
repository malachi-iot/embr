set(COMPONENT_SRCS "main.cpp" "udp-echo.cpp" "process.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

component_compile_options("-Wstack-usage=64")
#component_compile_options("-Wstack-usage=64" "-S")
