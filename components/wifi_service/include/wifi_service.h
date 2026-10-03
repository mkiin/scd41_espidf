#pragma once

#include <stdint.h>

#include "esp_err.h"

#define WIFI_SERVICE_IPV4_READY (UINT32_C(1) << 0)

// Prepare Wi-Fi without starting it; OSAL events are not required yet.
esp_err_t wifi_service_init(void);

// Call once after wifi_service_init() and creation of OSAL_EVENT_NETWORK.
// Starts Wi-Fi; connection readiness is reported via WIFI_SERVICE_IPV4_READY.
esp_err_t wifi_service_start(void);
