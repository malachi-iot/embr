set(COMPONENT_SRCS "dummy.cpp" "pbuf.cpp")
#set(COMPONENT_ADD_INCLUDEDIRS "include")

# Include the components directory of the main application:
#
set(EXTRA_COMPONENT_DIRS "../components/embr")

set(COMPONENT_REQUIRES embr unity testable)

register_component()