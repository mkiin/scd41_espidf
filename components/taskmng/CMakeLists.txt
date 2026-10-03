idf_component_register(
    SRCS "taskmng.c"
INCLUDE_DIRS "include"
    REQUIRES freertos
    PRIV_REQUIRES env_measure env_telemetory
)
