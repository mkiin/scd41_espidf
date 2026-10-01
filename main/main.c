#include <stdbool.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scd4x_driver.h"

static const char *TAG = "scd41";

static void scd41_task(void *arg)
{
    (void)arg;
    esp_err_t err = scd4x_driver_init();
    if ( err != ESP_OK )
    {
        ESP_LOGE(TAG, "init failed: %s", esp_err_to_name(err));
        goto failed;
    }

    // Start with the SCD41 powered normally, not in power-down mode.
    vTaskDelay(pdMS_TO_TICKS(1000) + 1);

    // Also handles an ESP32-only reboot while the sensor is still measuring.
    err = scd4x_driver_stop_periodic_measurement();
    if ( err != ESP_OK )
    {
        ESP_LOGE(TAG, "stop failed: %s", esp_err_to_name(err));
        goto failed;
    }
    // The official driver waits 500 ms through our HAL before returning.
    err = scd4x_driver_start_periodic_measurement();
    if ( err != ESP_OK )
    {
        ESP_LOGE(TAG, "start failed: %s", esp_err_to_name(err));
        goto failed;
    }
    ESP_LOGI(TAG, "Periodic measurement started; updates every ~5 s");

    TickType_t last_wake = xTaskGetTickCount();
    for ( ;; )
    {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000));

        bool ready = false;
        err        = scd4x_driver_get_data_ready(&ready);
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

        ESP_LOGI(
            TAG,
            "CO2=%u ppm, T=%.2f C, RH=%.2f %%",
            (unsigned)measurement.co2_ppm,
            measurement.temperature_mdeg_c / 1000.0,
            measurement.humidity_mpercent_rh / 1000.0);
    }

failed:
    ESP_LOGE(TAG, "Sensor task stopped; check wiring and power, then reset");
    err = scd4x_driver_deinit();
    if ( err != ESP_OK )
    {
        ESP_LOGE(TAG, "deinit failed: %s", esp_err_to_name(err));
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    if ( xTaskCreate(scd41_task, "scd41", 4096, NULL, 5, NULL) != pdPASS )
    {
        ESP_LOGE(TAG, "Could not create sensor task");
    }
}
