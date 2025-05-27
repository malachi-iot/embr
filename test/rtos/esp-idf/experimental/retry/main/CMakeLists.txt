idf_component_register(SRCS "retry.cpp" "wifi.cpp"
    INCLUDE_DIRS "."
    REQUIRES embr-dev esp_wifi nvs_flash
    )
