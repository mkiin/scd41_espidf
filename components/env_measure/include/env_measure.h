#pragma once

#include "esp_err.h"

esp_err_t env_measure_init(void);
void      env_measure_run(void *arg);
