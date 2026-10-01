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

static esp_err_t scd4x_driver_read_measurement(scd4x_measurement_t *measurement)
{
    return scd4x_error_to_esp(scd4x_read_measurement(&measurement->co2_ppm, &measurement->temperature_mdeg_c, &measurement->humidity_mpercent_rh));
}

esp_err_t scd4x_driver_try_read_measurement(scd4x_measurement_t *measurement)
{
    if ( measurement == NULL )
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

    return scd4x_driver_read_measurement(measurement);
}
