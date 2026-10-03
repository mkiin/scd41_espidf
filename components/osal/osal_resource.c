#include "env_measurement.h"
#include "osal_resource_internal.h"

osal_task_resource_t osal_task_resources[OSAL_TASK_COUNT] = {
    [OSAL_TASK_ENV_MEASURE] = {
        .id = OSAL_TASK_ENV_MEASURE,
        .name = "env_measure",
        .stack_size_bytes = 4096,
        .priority = 5,
        .handle = NULL,
    },
    [OSAL_TASK_ENV_TELEMETRY] = {
        .id = OSAL_TASK_ENV_TELEMETRY,
        .name = "env_telemetory",
        .stack_size_bytes = 4096,
        .priority = 5,
        .handle = NULL,
    },
};

osal_queue_resource_t osal_queue_resources[OSAL_QUEUE_COUNT] = {
    [OSAL_QUEUE_ENV_MEASUREMENT] = {
        .id = OSAL_QUEUE_ENV_MEASUREMENT,
        .length = 8,
        .item_size = sizeof(env_measurement_t),
        .handle = NULL,
    },
};
