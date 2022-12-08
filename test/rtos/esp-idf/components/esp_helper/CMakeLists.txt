set(COMPONENT_SRCS "esp-helper.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES nvs_flash embr estdlib esp_event esp_wifi)

register_component()