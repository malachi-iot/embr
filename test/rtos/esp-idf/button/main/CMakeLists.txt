idf_component_register(SRCS "isr.cpp" "main.cpp" "polled.cpp" "gpio.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES embr esp_helper
                    )
