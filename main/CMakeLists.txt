idf_component_register(
    SRCS "main.c" "app_platform.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES osal taskmng nvs_flash wifi_service
)
