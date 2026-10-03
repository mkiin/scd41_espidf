#include "app_platform.h"

#include "nvs_flash.h"
#include "wifi_service.h"

esp_err_t app_platform_init(void)
{
    esp_err_t err = nvs_flash_init();
    if ( err != ESP_OK )
    {
        return err;
    }

    return wifi_service_init();
}
