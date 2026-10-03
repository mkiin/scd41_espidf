#include "env_measure.h"

#include <stdbool.h>

#include "env_measurement.h"
#include "esp_log.h"
#include "osal_queue.h"
#include "osal_task.h"
#include "scd4x_driver.h"

#define ENV_MEASURE_POLL_INTERVAL_MS 5000U

static const char *TAG = "env_measure";

static bool env_measure_try_read(env_measurement_t *sample)
{
    scd4x_measurement_t measurement;

    esp_err_t err = scd4x_driver_try_read_measurement(&measurement);

    if ( err == ESP_ERR_NOT_FINISHED )
    {
        return false;
    }

    if ( err != ESP_OK )
    {
        ESP_LOGW(TAG, "read failed: %s", esp_err_to_name(err));
        return false;
    }

    if ( measurement.co2_ppm == 0 )
    {
        ESP_LOGW(TAG, "Ignoring invalid CO2 sample (0 ppm)");
        return false;
    }

    *sample = (env_measurement_t){
        .co2_ppm              = measurement.co2_ppm,
        .temperature_mdeg_c   = measurement.temperature_mdeg_c,
        .humidity_mpercent_rh = measurement.humidity_mpercent_rh,
    };
    return true;
}

static void env_measure_publish(const env_measurement_t *sample)
{
    esp_err_t err = osal_queue_overwrite(OSAL_QUEUE_ENV_MEASUREMENT, sample);

    if ( err != ESP_OK )
    {
        ESP_LOGW(TAG, "sample not queued: %s", esp_err_to_name(err));

        return;
    }
    ESP_LOGI(TAG, "CO2=%u ppm, T=%.2f C, RH=%.2f %%", (unsigned)sample->co2_ppm, sample->temperature_mdeg_c / 1000.0, sample->humidity_mpercent_rh / 1000.0);
}

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

    osal_task_period_t period;
    osal_task_period_init(&period);

    for ( ;; )
    {
        osal_task_delay_until(&period, ENV_MEASURE_POLL_INTERVAL_MS);
        env_measurement_t sample;

        if ( !env_measure_try_read(&sample) )
        {
            continue;
        }
        env_measure_publish(&sample);
    }
}
