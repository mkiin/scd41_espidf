#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "freertos/event_groups.h"

#define WIFI_SERVICE_IPV4_READY (UINT32_C(1) << 0)

// Prepare Wi-Fi without starting it.
esp_err_t wifi_service_init(EventGroupHandle_t network_event_group);

// Start/stop are called sequentially by the telemetry task only, using the
// event group passed to init. Call stop even when start/connect fails.
esp_err_t wifi_service_start(EventGroupHandle_t network_event_group);
// Waits for the stop event before returning ESP_OK.
esp_err_t wifi_service_stop(EventGroupHandle_t network_event_group);
