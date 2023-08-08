idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES embr esp_helper driver esp_timer perfmon
                    )
