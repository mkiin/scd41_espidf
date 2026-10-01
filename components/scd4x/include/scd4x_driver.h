#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct
{
    uint16_t co2_ppm;
    int32_t  temperature_mdeg_c;
    int32_t  humidity_mpercent_rh;
} scd4x_measurement_t;

esp_err_t scd4x_driver_init(void);
esp_err_t scd4x_driver_deinit(void);

esp_err_t scd4x_driver_start_periodic_measurement(void);
esp_err_t scd4x_driver_stop_periodic_measurement(void);

esp_err_t scd4x_driver_get_data_ready(bool *ready);
esp_err_t scd4x_driver_read_measurement(scd4x_measurement_t *measurement);
