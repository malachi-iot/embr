idf_component_register(SRCS "main.cpp" "ws2812.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES embr esp_helper led_strip
                    )
