idf_component_register(SRCS "embr-storage.c"
    INCLUDE_DIRS "include"
    # 04MAY25 MB - A little odd that I need spi_flash in addition to esp_partition, but there you have it
    REQUIRES esp_partition spi_flash
    )
