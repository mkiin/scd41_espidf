#include "taskmng.h"

#include <stddef.h>

#include "env_measure.h"
#include "osal_resource.h"
#include "osal_task.h"

typedef esp_err_t (*taskmng_init_t)(void);

typedef struct
{
    osal_task_id_t    id;
    taskmng_init_t    init;
    osal_task_entry_t run;
    void             *arg;
} taskmng_entry_t;

static const taskmng_entry_t s_task_entries[] = {
    {
     .id   = OSAL_TASK_ENV_MEASURE,
     .init = env_measure_init,
     .run  = env_measure_run,
     .arg  = NULL,
     },
};

esp_err_t taskmng_init_all(void)
{
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

        esp_err_t err = osal_task_create(s_task_entries[ i ].id, s_task_entries[ i ].run, s_task_entries[ i ].arg);
        if ( err != ESP_OK )
        {
            return err;
        }
    }

    return ESP_OK;
}
