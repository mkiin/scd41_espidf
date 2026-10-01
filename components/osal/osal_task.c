#include "osal_task.h"

#include <stddef.h>

#include "osal_resource_internal.h"

static osal_task_resource_t *osal_task_get_by(osal_task_id_t id)
{
    if ( (unsigned)id >= OSAL_TASK_COUNT )
    {
        return NULL;
    }

    return &osal_task_resources[ id ];
}

esp_err_t osal_task_create(osal_task_id_t id, osal_task_entry_t entry, void *arg)
{
    if ( entry == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    osal_task_resource_t *resource = osal_task_get_by(id);
    if ( resource == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ( resource->handle != NULL )
    {
        return ESP_ERR_INVALID_STATE;
    }

    BaseType_t result = xTaskCreate(entry, resource->name, resource->stack_size_bytes, arg, resource->priority, &resource->handle);
    if ( result != pdPASS )
    {
        resource->handle = NULL;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void osal_task_period_init(osal_task_period_t *period)
{
    period->last_wake_tick = (uint64_t)xTaskGetTickCount();
}

void osal_task_delay_until(osal_task_period_t *period, uint32_t interval_ms)
{
    TickType_t last_wake = (TickType_t)period->last_wake_tick;

    TickType_t interval = pdMS_TO_TICKS(interval_ms);

    (void)xTaskDelayUntil(&last_wake, interval);

    period->last_wake_tick = (uint64_t)last_wake;
}

void osal_task_delay_ms(uint32_t delay_ms)
{
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
}
