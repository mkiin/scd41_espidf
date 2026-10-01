#pragma once

#include "esp_err.h"
#include "osal_resource.h"

typedef void (*osal_task_entry_t)(void *arg);

esp_err_t osal_task_create(osal_task_id_t id, osal_task_entry_t entry, void *arg);
