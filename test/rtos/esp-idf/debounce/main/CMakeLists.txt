idf_component_register(SRCS "init.cpp" "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES embr esp_helper esp_timer perfmon
                    )
