idf_component_register(SRCS "embr-storage.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_partition wear_levelling
    )
