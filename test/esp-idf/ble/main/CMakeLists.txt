idf_component_register(SRCS "ble.cpp" "uuid.cpp"
    INCLUDE_DIRS "."
    REQUIRES bt embr
    )

#set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
