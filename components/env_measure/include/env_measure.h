#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "freertos/queue.h"

typedef struct
{
    uint16_t co2_ppm;
    int32_t  temperature;
    int32_t  humidity;
} env_measurement_t;

typedef struct
{
    QueueHandle_t measurement_queue;
} env_measure_args_t;

esp_err_t env_measure_init(void);
void      env_measure_run(void *arg);
