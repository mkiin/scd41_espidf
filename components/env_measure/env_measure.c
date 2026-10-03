#include "env_measure.h"

#include <stdbool.h>

#include "esp_log.h"
#include "esp_pm.h"
#include "freertos/task.h"
#include "scd4x_driver.h"
#include "sdkconfig.h"

#define ENV_MEASURE_INTERVAL_MS (CONFIG_APP_MEASUREMENT_INTERVAL_SECONDS * 1000U)

static const char          *TAG = "env_measure";
static esp_pm_lock_handle_t s_pm_lock;

static bool env_measure_try_read(env_measurement_t *sample)
{
    esp_err_t err = scd4x_driver_measure_single_shot(&sample->co2_ppm, &sample->temperature, &sample->humidity);

    if ( err == ESP_ERR_NOT_FINISHED )
    {
        return false;
    }

    if ( err != ESP_OK )
    {
        ESP_LOGW(TAG, "read failed: %s", esp_err_to_name(err));
        return false;
    }

    if ( sample->co2_ppm == 0 )
    {
        ESP_LOGW(TAG, "Ignoring invalid CO2 sample (0 ppm)");
        return false;
    }

    return true;
}

static void env_measure_publish(QueueHandle_t measurement_queue, const env_measurement_t *sample)
{
    if ( xQueueOverwrite(measurement_queue, sample) != pdPASS )
    {
        ESP_LOGW(TAG, "sample not queued");
        return;
    }
    ESP_LOGI(TAG, "CO2=%u ppm, T=%.2f C, RH=%.2f %%", (unsigned)sample->co2_ppm, sample->temperature / 1000.0, sample->humidity / 1000.0);
}

esp_err_t env_measure_init(void)
{
    esp_err_t err = scd4x_driver_init();
    if ( err != ESP_OK )
    {
        return err;
    }

    // Allow an in-flight single shot to finish after an ESP32-only reboot.
    vTaskDelay(pdMS_TO_TICKS(5100));
    // Wake-up has no ACK; periodic mode may reject the serial-number read.
    (void)scd4x_driver_wake_up();
    err = scd4x_driver_stop_periodic_measurement();
    if ( err != ESP_OK )
    {
        return err;
    }

    err = scd4x_driver_disable_automatic_calibration();
    if ( err != ESP_OK )
    {
        return err;
    }

    err = scd4x_driver_power_down();
    if ( err != ESP_OK )
    {
        return err;
    }
    ESP_LOGI(TAG, "Single-shot interval: %u seconds; ASC disabled", (unsigned)CONFIG_APP_MEASUREMENT_INTERVAL_SECONDS);
    return esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "measurement", &s_pm_lock);
}

void env_measure_run(void *arg)
{
    env_measure_args_t *args = (env_measure_args_t *)arg;

    TickType_t last_wake_tick = xTaskGetTickCount();

    for ( ;; )
    {
        ESP_ERROR_CHECK(esp_pm_lock_acquire(s_pm_lock));
        env_measurement_t sample;
        esp_err_t         err   = scd4x_driver_wake_up();
        bool              valid = false;
        if ( err == ESP_OK )
        {
            valid = env_measure_try_read(&sample);
        }
        else
        {
            ESP_LOGW(TAG, "Sensor wake failed: %s", esp_err_to_name(err));
        }
        err = scd4x_driver_power_down();
        if ( err != ESP_OK )
        {
            ESP_LOGE(TAG, "Sensor power-down failed: %s; sensor may remain powered", esp_err_to_name(err));
        }
        if ( valid )
        {
            env_measure_publish(args->measurement_queue, &sample);
        }
        ESP_ERROR_CHECK(esp_pm_lock_release(s_pm_lock));
        xTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(ENV_MEASURE_INTERVAL_MS));
    }
}
