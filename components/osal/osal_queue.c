#include "osal_queue.h"

#include <stddef.h>

#include "osal_resource_internal.h"

static osal_queue_resource_t *osal_queue_get_by(osal_queue_id_t id)
{
    if ( (unsigned)id >= OSAL_QUEUE_COUNT )
    {
        return NULL;
    }

    return &osal_queue_resources[ id ];
}

static TickType_t osal_queue_timeout_ticks(uint32_t timeout_ms)
{
    if ( timeout_ms == OSAL_WAIT_FOREVER )
    {
        return portMAX_DELAY;
    }

    // Round positive timeouts up, using 64 bits to avoid overflow.
    uint64_t ticks = ((uint64_t)timeout_ms * configTICK_RATE_HZ + 999ULL) / 1000ULL;
    // Reserve portMAX_DELAY for an explicitly indefinite wait.
    if ( ticks >= portMAX_DELAY )
    {
        return portMAX_DELAY - 1;
    }

    return (TickType_t)ticks;
}

esp_err_t osal_queue_create(osal_queue_id_t id)
{
    osal_queue_resource_t *resource = osal_queue_get_by(id);
    if ( resource == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ( resource->handle != NULL )
    {
        return ESP_ERR_INVALID_STATE;
    }

    resource->handle = xQueueCreate(resource->length, resource->item_size);
    if ( resource->handle == NULL )
    {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t osal_queue_create_all(void)
{
    for ( osal_queue_id_t id = 0; id < OSAL_QUEUE_COUNT; ++id )
    {
        esp_err_t err = osal_queue_create(id);
        if ( err != ESP_OK )
        {
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t osal_queue_send(osal_queue_id_t id, const void *item, uint32_t timeout_ms)
{
    osal_queue_resource_t *resource = osal_queue_get_by(id);
    if ( resource == NULL || item == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ( resource->handle == NULL )
    {
        return ESP_ERR_INVALID_STATE;
    }

    if ( xQueueSend(resource->handle, item, osal_queue_timeout_ticks(timeout_ms)) != pdPASS )
    {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t osal_queue_receive(osal_queue_id_t id, void *item, uint32_t timeout_ms)
{
    osal_queue_resource_t *resource = osal_queue_get_by(id);
    if ( resource == NULL || item == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ( resource->handle == NULL )
    {
        return ESP_ERR_INVALID_STATE;
    }

    if ( xQueueReceive(resource->handle, item, osal_queue_timeout_ticks(timeout_ms)) != pdPASS )
    {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}
