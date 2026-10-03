#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "freertos/event_groups.h"
#include "freertos/queue.h"

esp_err_t          taskmng_init_all(QueueHandle_t measurement_queue, EventGroupHandle_t network_event_group);
esp_err_t          taskmng_create_all(void);
EventGroupHandle_t taskmng_get_network_event_group(void);
