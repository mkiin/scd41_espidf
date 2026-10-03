#include "taskmng.h"

#include <stddef.h>

#include "env_measure.h"
#include "env_telemetory.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef struct
{
    const char *name;
    uint32_t    stack_size_bytes;
    UBaseType_t priority;
    esp_err_t (*init)(void);
    TaskFunction_t run;
    void          *arg;
    TaskHandle_t   handle;
} taskmng_entry_t;

static env_measure_args_t    s_env_measure_args;
static env_telemetory_args_t s_env_telemetory_args;

static taskmng_entry_t s_task_entries[] = {
    {
     .name             = "env_measure",
     .stack_size_bytes = 4096,
     .priority         = 5,
     .init             = env_measure_init,
     .run              = env_measure_run,
     .arg              = &s_env_measure_args,
     .handle           = NULL,
     },
    {
     .name             = "env_telemetory",
     .stack_size_bytes = 4096,
     .priority         = 5,
     .init             = env_telemetory_init,
     .run              = env_telemetory_run,
     .arg              = &s_env_telemetory_args,
     .handle           = NULL,
     }
};

esp_err_t taskmng_init_all(QueueHandle_t measurement_queue, EventGroupHandle_t network_event_group)
{
    if ( measurement_queue == NULL || network_event_group == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }

    s_env_measure_args.measurement_queue = measurement_queue;

    s_env_telemetory_args.measurement_queue   = measurement_queue;
    s_env_telemetory_args.network_event_group = network_event_group;

    for ( size_t i = 0; i < sizeof(s_task_entries) / sizeof(s_task_entries[ 0 ]); ++i )
    {
        if ( s_task_entries[ i ].init == NULL )
        {
            continue;
        }

        esp_err_t err = s_task_entries[ i ].init();
        if ( err != ESP_OK )
        {
            return err;
        }
    }

    return ESP_OK;
}

esp_err_t taskmng_create_all(void)
{
    for ( size_t i = 0; i < sizeof(s_task_entries) / sizeof(s_task_entries[ 0 ]); ++i )
    {
        if ( s_task_entries[ i ].run == NULL )
        {
            return ESP_ERR_INVALID_STATE;
        }

        taskmng_entry_t *entry = &s_task_entries[ i ];
        if ( entry->handle != NULL )
        {
            return ESP_ERR_INVALID_STATE;
        }

        BaseType_t result = xTaskCreate(entry->run, entry->name, entry->stack_size_bytes, entry->arg, entry->priority, &entry->handle);
        if ( result != pdPASS )
        {
            entry->handle = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    return ESP_OK;
}
