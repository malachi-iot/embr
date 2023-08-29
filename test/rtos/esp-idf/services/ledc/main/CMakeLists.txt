idf_component_register(SRCS "main.cpp" "services-iram.cpp"
    INCLUDE_DIRS "."
    REQUIRES embr esp_helper nvs_flash
    LDFRAGMENTS iram.lf
    )
