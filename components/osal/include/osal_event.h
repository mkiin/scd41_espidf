#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "osal_resource.h"

esp_err_t osal_event_create(osal_event_id_t id);

esp_err_t osal_event_set(osal_event_id_t id, uint32_t bits);

esp_err_t osal_event_clear(osal_event_id_t id, uint32_t bits);

esp_err_t osal_event_wait_all(osal_event_id_t id, uint32_t bits, uint32_t timeout_ms);
