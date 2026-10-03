#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"  // IWYU pragma: keep
#include "freertos/event_groups.h"

#define WIFI_SERVICE_IPV4_READY (UINT32_C(1) << 0)

// Prepare Wi-Fi without starting it.
esp_err_t wifi_service_init(EventGroupHandle_t network_event_group);
