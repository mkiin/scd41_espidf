#include "power_management.h"

#include "esp_pm.h"
#include "sdkconfig.h"

esp_err_t power_management_init(void)
{
    const esp_pm_config_t config = {
        .max_freq_mhz       = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz       = 40,
        .light_sleep_enable = true,
    };
    return esp_pm_configure(&config);
}
