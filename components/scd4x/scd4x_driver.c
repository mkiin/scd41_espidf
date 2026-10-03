#include "scd4x_driver.h"

#include <stdbool.h>
#include <stddef.h>

#include "scd4x_hal_internal.h"
#include "scd4x_i2c.h"

static esp_err_t scd4x_error_to_esp(int16_t error)
{
    return error == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t scd4x_driver_init(void)
{
    esp_err_t err = scd4x_hal_create();
    if ( err != ESP_OK )
    {
        return err;
    }

    scd4x_init(SCD41_I2C_ADDR_62);
    return ESP_OK;
}

esp_err_t scd4x_driver_deinit(void)
{
    return scd4x_hal_destroy();
}

esp_err_t scd4x_driver_start_periodic_measurement(void)
{
    return scd4x_error_to_esp(scd4x_start_periodic_measurement());
}

esp_err_t scd4x_driver_stop_periodic_measurement(void)
{
    return scd4x_error_to_esp(scd4x_stop_periodic_measurement());
}

static esp_err_t scd4x_driver_get_data_ready(bool *ready)
{
    return scd4x_error_to_esp(scd4x_get_data_ready_status(ready));
}

esp_err_t scd4x_driver_wake_up(void)
{
    // No ACK is expected; the vendor function includes the wake-up delay.
    scd4x_wake_up();
    uint16_t serial[ 3 ];
    return scd4x_error_to_esp(scd4x_get_serial_number(serial, 3));
}

esp_err_t scd4x_driver_power_down(void)
{
    return scd4x_error_to_esp(scd4x_power_down());
}

esp_err_t scd4x_driver_disable_automatic_calibration(void)
{
    // ASC is unavailable when powering down between single-shot measurements.
    return scd4x_error_to_esp(scd4x_set_automatic_self_calibration_enabled(0));
}

esp_err_t scd4x_driver_measure_single_shot(uint16_t *co2_ppm, int32_t *temperature, int32_t *humidity)
{
    if ( co2_ppm == NULL || temperature == NULL || humidity == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = scd4x_error_to_esp(scd4x_measure_single_shot());
    if ( err != ESP_OK )
    {
        return err;
    }
    return scd4x_error_to_esp(scd4x_read_measurement(co2_ppm, temperature, humidity));
}

esp_err_t scd4x_driver_try_read_measurement(uint16_t *co2_ppm, int32_t *temperature, int32_t *humidity)
{
    if ( co2_ppm == NULL || temperature == NULL || humidity == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    bool      ready = false;
    esp_err_t err   = scd4x_driver_get_data_ready(&ready);
    if ( err != ESP_OK )
    {
        return err;
    }

    if ( !ready )
    {
        return ESP_ERR_NOT_FINISHED;
    }

    return scd4x_error_to_esp(scd4x_read_measurement(co2_ppm, temperature, humidity));
}
