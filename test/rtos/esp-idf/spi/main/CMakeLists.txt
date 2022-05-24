set(COMPONENT_SRCS "main.cpp" "spi.cpp" "lcd.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

component_compile_options("-Wstack-usage=256")
#component_compile_options("-Wstack-usage=64" "-S")
