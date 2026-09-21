#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "scd4x_i2c.h"
#include "sensirion_i2c_hal.h"

static const char *TAG = "scd41";

static void scd41_task(void *arg)
{
    (void)arg;
    sensirion_i2c_hal_init();
    scd4x_init(0x62);

    // Start with the SCD41 powered normally, not in power-down mode.
    sensirion_i2c_hal_sleep_usec(1000000);

    // Also handles an ESP32-only reboot while the sensor is still measuring.
    int16_t err = scd4x_stop_periodic_measurement();
    if ( err != 0 )
    {
        ESP_LOGE(TAG, "stop failed: %d", (int)err);
        goto failed;
    }
    // The official driver waits 500 ms through our HAL before returning.
    err = scd4x_start_periodic_measurement();
    if ( err != 0 )
    {
        ESP_LOGE(TAG, "start failed: %d", (int)err);
        goto failed;
    }
    ESP_LOGI(TAG, "Periodic measurement started; updates every ~5 s");

    TickType_t last_wake = xTaskGetTickCount();
    for ( ;; )
    {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000));

        bool ready = false;
        err        = scd4x_get_data_ready_status(&ready);
        if ( err != 0 )
        {
            ESP_LOGW(TAG, "status failed: %d", (int)err);
            continue;
        }
        if ( !ready )
        {
            continue;
        }

        uint16_t co2;
        int32_t  temperature_mdeg;
        int32_t  humidity_mpercent;
        err = scd4x_read_measurement(&co2, &temperature_mdeg, &humidity_mpercent);
        if ( err != 0 )
        {
            ESP_LOGW(TAG, "read failed: %d", (int)err);
            continue;
        }
        if ( co2 == 0 )
        {
            ESP_LOGW(TAG, "Ignoring invalid CO2 sample (0 ppm)");
            continue;
        }

        ESP_LOGI(TAG, "CO2=%u ppm, T=%.2f C, RH=%.2f %%", (unsigned)co2, temperature_mdeg / 1000.0, humidity_mpercent / 1000.0);
    }

failed:
    ESP_LOGE(TAG, "Sensor task stopped; check wiring and power, then reset");
    sensirion_i2c_hal_free();
    vTaskDelete(NULL);
}

void app_main(void)
{
    if ( xTaskCreate(scd41_task, "scd41", 4096, NULL, 5, NULL) != pdPASS )
    {
        ESP_LOGE(TAG, "Could not create sensor task");
    }
}
