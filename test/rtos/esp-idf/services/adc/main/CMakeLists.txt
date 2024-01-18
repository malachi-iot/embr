idf_component_register(SRCS "main.cpp" "services-iram.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_adc embr esp_helper driver
                    LDFRAGMENTS iram.lf
                    )
