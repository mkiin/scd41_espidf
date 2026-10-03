#include "env_measure.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "taskmng.h"
#include "wifi_service.h"

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if ( ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND )
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    QueueHandle_t measurement_queue = xQueueCreate(1, sizeof(env_measurement_t));
    ESP_ERROR_CHECK(measurement_queue != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    EventGroupHandle_t network_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(network_event_group != NULL ? ESP_OK : ESP_ERR_NO_MEM);

    ESP_ERROR_CHECK(taskmng_init_all(measurement_queue, network_event_group));

    ESP_ERROR_CHECK(wifi_service_init(network_event_group));

    ESP_ERROR_CHECK(taskmng_create_all());
}
