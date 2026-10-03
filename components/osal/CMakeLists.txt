idf_component_register(
    SRCS
        "osal_resource.c"
        "osal_task.c"
        "osal_queue.c"
        "osal_event.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    PRIV_REQUIRES freertos app_types
)
