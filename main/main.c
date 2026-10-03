#include "app_platform.h"
#include "esp_err.h"
#include "osal_event.h"
#include "osal_queue.h"
#include "taskmng.h"
#include "wifi_service.h"

void app_main(void)
{
    ESP_ERROR_CHECK(app_platform_init());
    ESP_ERROR_CHECK(osal_queue_create_all());
    ESP_ERROR_CHECK(osal_event_create(OSAL_EVENT_NETWORK));
    ESP_ERROR_CHECK(wifi_service_start());
    ESP_ERROR_CHECK(taskmng_init_all());
    ESP_ERROR_CHECK(taskmng_create_all());
}
