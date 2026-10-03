#pragma once

#include "esp_err.h"

// Enable ESP-IDF automatic light sleep. Tasks use esp_pm locks directly.
esp_err_t power_management_init(void);
