#include "app_platform.h"

#include "nvs_flash.h"

esp_err_t app_platform_init(void)
{
    return nvs_flash_init();
}
