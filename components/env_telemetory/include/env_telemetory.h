#pragma once
#include "esp_err.h"
#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "freertos/event_groups.h"
#include "freertos/queue.h"

typedef struct
{
    QueueHandle_t      measurement_queue;
    EventGroupHandle_t network_event_group;
} env_telemetory_args_t;

esp_err_t env_telemetory_init(void);
void      env_telemetory_run(void *arg);
