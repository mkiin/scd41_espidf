#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "osal_resource.h"
#include "osal_types.h"

typedef struct
{
    osal_task_id_t id;
    const char    *name;
    uint32_t       stack_size_bytes;
    UBaseType_t    priority;
    TaskHandle_t   handle;
} osal_task_resource_t;

typedef struct
{
    osal_queue_id_t id;
    UBaseType_t     length;
    UBaseType_t     item_size;
    QueueHandle_t   handle;
} osal_queue_resource_t;

typedef struct
{
    EventGroupHandle_t handle;
} osal_event_resource_t;

extern osal_task_resource_t  osal_task_resources[ OSAL_TASK_COUNT ];
extern osal_queue_resource_t osal_queue_resources[ OSAL_QUEUE_COUNT ];
extern osal_event_resource_t osal_event_resources[ OSAL_EVENT_COUNT ];
