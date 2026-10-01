#include "env_measure.h"

#include <stdbool.h>

#include "env_measurement.h"
#include "esp_log.h"
#include "osal_queue.h"
#include "scd4x_driver.h"

static const char *TAG = "env_measure";

esp_err_t env_measure_init(void)
{
    esp_err_t err = scd4x_driver_init();
    if ( err != ESP_OK )
    {
        return err;
    }

    // Also handles an ESP32-only reboot while the sensor is still measuring.
    err = scd4x_driver_stop_periodic_measurement();
    if ( err != ESP_OK )
    {
        return err;
    }

    err = scd4x_driver_start_periodic_measurement();
    if ( err != ESP_OK )
    {
        return err;
    }

    ESP_LOGI(TAG, "Periodic measurement started; updates every ~5 s");
    return ESP_OK;
}

void env_measure_run(void *arg)
{
    (void)arg;

    for ( ;; )
    {
        bool      ready = false;
        esp_err_t err   = scd4x_driver_get_data_ready(&ready);
        if ( err != ESP_OK )
        {
            ESP_LOGW(TAG, "status failed: %s", esp_err_to_name(err));
            continue;
        }
        if ( !ready )
        {
            continue;
        }

        scd4x_measurement_t measurement;
        err = scd4x_driver_read_measurement(&measurement);
        if ( err != ESP_OK )
        {
            ESP_LOGW(TAG, "read failed: %s", esp_err_to_name(err));
            continue;
        }
        if ( measurement.co2_ppm == 0 )
        {
            ESP_LOGW(TAG, "Ignoring invalid CO2 sample (0 ppm)");
            continue;
        }

        env_measurement_t sample = {
            .co2_ppm              = measurement.co2_ppm,
            .temperature_mdeg_c   = measurement.temperature_mdeg_c,
            .humidity_mpercent_rh = measurement.humidity_mpercent_rh,
        };
        // Keep acquiring samples even when no telemetry consumer is running.
        err = osal_queue_send(OSAL_QUEUE_ENV_MEASUREMENT, &sample, 0);
        if ( err != ESP_OK )
        {
            ESP_LOGW(TAG, "Sample not queued: %s", esp_err_to_name(err));
        }

        ESP_LOGI(TAG, "CO2=%u ppm, T=%.2f C, RH=%.2f %%", (unsigned)sample.co2_ppm, sample.temperature_mdeg_c / 1000.0, sample.humidity_mpercent_rh / 1000.0);
    }
}
