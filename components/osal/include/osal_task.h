#pragma once

#include "esp_err.h"
#include "osal_resource.h"

typedef void (*osal_task_entry_t)(void *arg);

typedef struct
{
    uint64_t last_wake_tick;
} osal_task_period_t;

esp_err_t osal_task_create(osal_task_id_t id, osal_task_entry_t entry, void *arg);
void      osal_task_period_init(osal_task_period_t *period);
void      osal_task_delay_until(osal_task_period_t *period, uint32_t interval_ms);
void      osal_task_delay_ms(uint32_t delay_ms);
