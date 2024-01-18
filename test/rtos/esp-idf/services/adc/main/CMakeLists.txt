idf_component_register(SRCS "init.cpp" "main.cpp" "services-iram.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_adc embr esp_helper driver
                    LDFRAGMENTS iram.lf
                    )
