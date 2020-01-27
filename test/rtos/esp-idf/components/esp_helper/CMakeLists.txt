set(COMPONENT_SRCS "esp-helper.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES nvs_flash embr estdlib)

register_component()