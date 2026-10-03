#include "osal_event.h"

#include "osal_resource_internal.h"

/* ESP-IDF v5.5.2 / ESP32-S3用。上位8ビットは使用しない。 */
#define OSAL_EVENT_VALID_BITS UINT32_C(0x00FFFFFF)

static osal_event_resource_t *get_resource(osal_event_id_t id)
{
    if ( (unsigned)id >= (unsigned)OSAL_EVENT_COUNT )
    {
        return NULL;
    }

    return &osal_event_resources[ id ];
}

static esp_err_t get_handle(osal_event_id_t id, uint32_t bits, EventGroupHandle_t *handle)
{
    osal_event_resource_t *resource = get_resource(id);

    if ( resource == NULL || bits == 0 || (bits & ~OSAL_EVENT_VALID_BITS) != 0 )
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ( resource->handle == NULL )
    {
        return ESP_ERR_INVALID_STATE;
    }

    *handle = resource->handle;
    return ESP_OK;
}

esp_err_t osal_event_create(osal_event_id_t id)
{
    osal_event_resource_t *resource = get_resource(id);

    if ( resource == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    if ( resource->handle != NULL )
    {
        return ESP_ERR_INVALID_STATE;
    }

    resource->handle = xEventGroupCreate();

    return resource->handle != NULL ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t osal_event_set(osal_event_id_t id, uint32_t bits)
{
    EventGroupHandle_t handle;
    esp_err_t          err = get_handle(id, bits, &handle);
    if ( err != ESP_OK )
    {
        return err;
    }

    xEventGroupSetBits(handle, (EventBits_t)bits);
    return ESP_OK;
}

esp_err_t osal_event_clear(osal_event_id_t id, uint32_t bits)
{
    EventGroupHandle_t handle;
    esp_err_t          err = get_handle(id, bits, &handle);
    if ( err != ESP_OK )
    {
        return err;
    }

    xEventGroupClearBits(handle, (EventBits_t)bits);
    return ESP_OK;
}

esp_err_t osal_event_wait_all(osal_event_id_t id, uint32_t bits, uint32_t timeout_ms)
{
    EventGroupHandle_t handle;
    esp_err_t          err = get_handle(id, bits, &handle);
    if ( err != ESP_OK )
    {
        return err;
    }

    EventBits_t observed = xEventGroupWaitBits(
        handle,
        (EventBits_t)bits,
        pdFALSE, /* ビットを保持 */
        pdTRUE,  /* 指定した全ビットを待つ */
        pdMS_TO_TICKS(timeout_ms));

    return (observed & (EventBits_t)bits) == (EventBits_t)bits ? ESP_OK : ESP_ERR_TIMEOUT;
}
