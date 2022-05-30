set(COMPONENT_SRCS "main.cpp" "scheduler.cpp" "scheduler_tasknotify.cpp" "scheduler_bruteforce.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

component_compile_options("-Wstack-usage=256")

# Won't link, only for diagnostic
#component_compile_options("-Wstack-usage=64" "-S")
