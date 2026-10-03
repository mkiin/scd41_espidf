#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "osal_resource.h"

esp_err_t osal_queue_create(osal_queue_id_t id);
esp_err_t osal_queue_create_all(void);

esp_err_t osal_queue_send(osal_queue_id_t id, const void *item, uint32_t timeout_ms);
esp_err_t osal_queue_receive(osal_queue_id_t id, void *item, uint32_t timeout_ms);

esp_err_t osal_queue_overwrite(osal_queue_id_t id, const void *item);
