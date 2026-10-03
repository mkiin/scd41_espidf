idf_component_register(
    SRCS "env_telemetory.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES osal app_types log wifi_service esp_http_client json
)
