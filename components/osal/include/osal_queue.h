#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "osal_resource.h"

esp_err_t osal_queue_create(osal_queue_id_t id);
esp_err_t osal_queue_create_all(void);

// timeout_ms: 0 for non-blocking, OSAL_WAIT_FOREVER for indefinite blocking.
// Send/receive return ESP_ERR_TIMEOUT if the queue cannot complete the operation.
esp_err_t osal_queue_send(osal_queue_id_t id, const void *item, uint32_t timeout_ms);
esp_err_t osal_queue_receive(osal_queue_id_t id, void *item, uint32_t timeout_ms);
