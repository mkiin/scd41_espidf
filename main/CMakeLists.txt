idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES taskmng env_measure nvs_flash wifi_service power_management
)
