#pragma once

#include <stdint.h>

#include "esp_err.h"

esp_err_t scd4x_driver_init(void);
esp_err_t scd4x_driver_deinit(void);

esp_err_t scd4x_driver_start_periodic_measurement(void);
esp_err_t scd4x_driver_stop_periodic_measurement(void);

// Wake-up is verified by reading the serial number (wake command has no ACK).
esp_err_t scd4x_driver_wake_up(void);
esp_err_t scd4x_driver_power_down(void);
esp_err_t scd4x_driver_disable_automatic_calibration(void);
// Blocks for the sensor's 5-second conversion, then reads all three outputs.
esp_err_t scd4x_driver_measure_single_shot(uint16_t *co2_ppm, int32_t *temperature, int32_t *humidity);

// ESP_OK: measurement acquired; ESP_ERR_NOT_FINISHED: no data ready yet.
// All output pointers must be non-NULL. Use outputs only on ESP_OK.
// Units: CO2 in ppm, temperature in millidegrees Celsius,
// humidity in thousandths of percent relative humidity.
esp_err_t scd4x_driver_try_read_measurement(uint16_t *co2_ppm, int32_t *temperature, int32_t *humidity);
