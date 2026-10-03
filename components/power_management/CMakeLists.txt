idf_component_register(
    SRCS "power_management.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common
    PRIV_REQUIRES esp_pm
)
