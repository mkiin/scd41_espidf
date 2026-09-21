idf_component_register(
    SRCS
        "vendor/scd4x_i2c.c"
        "vendor/sensirion_common.c"
        "vendor/sensirion_i2c.c"
        "sensirion_i2c_hal_espidf.c"
    INCLUDE_DIRS "vendor"
    PRIV_REQUIRES esp_driver_i2c freertos log
)
