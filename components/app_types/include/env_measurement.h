#pragma once

#include <stdint.h>

typedef struct
{
    uint16_t co2_ppm;
    int32_t  temperature_mdeg_c;
    int32_t  humidity_mpercent_rh;
} env_measurement_t;
