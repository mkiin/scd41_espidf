#include "env_telemetory.h"

#include <stdbool.h>
#include <string.h>

#include "cJSON.h"
#include "env_measure.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "sdkconfig.h"
#include "wifi_service.h"

#define ENV_TELEMETRY_WAIT_MS         (CONFIG_APP_NETWORK_WAIT_SECONDS * 1000U)
#define ENV_TELEMETRY_HTTP_TIMEOUT_MS 5000

static const char              *TAG = "env_telemetory";
static esp_http_client_handle_t s_client;
static esp_pm_lock_handle_t     s_pm_lock;

static char *env_telemetory_create_payload(const env_measurement_t *sample)
{
    cJSON *json = cJSON_CreateObject();
    if ( json == NULL )
    {
        return NULL;
    }

    if ( cJSON_AddNumberToObject(json, "co2_ppm", sample->co2_ppm) == NULL || cJSON_AddNumberToObject(json, "temperature", sample->temperature) == NULL ||
         cJSON_AddNumberToObject(json, "humidity", sample->humidity) == NULL )
    {
        cJSON_Delete(json);
        return NULL;
    }

    char *payload = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    return payload;
}

static void env_telemetory_send(const env_measurement_t *sample)
{
    char *payload = env_telemetory_create_payload(sample);
    if ( payload == NULL )
    {
        ESP_LOGE(TAG, "Could not allocate telemetry payload");
        return;
    }

    esp_err_t err = esp_http_client_set_post_field(s_client, payload, (int)strlen(payload));
    if ( err == ESP_OK )
    {
        err = esp_http_client_perform(s_client);
    }

    if ( err == ESP_OK )
    {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld", esp_http_client_get_status_code(s_client), esp_http_client_get_content_length(s_client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST 失敗: %s", esp_err_to_name(err));
    }
    esp_http_client_close(s_client);
    esp_http_client_set_post_field(s_client, NULL, 0);
    cJSON_free(payload);
}

esp_err_t env_telemetory_init(void)
{
    const esp_http_client_config_t config = {
        .url                   = CONFIG_APP_TELEMETRY_URL,
        .method                = HTTP_METHOD_POST,
        .timeout_ms            = ENV_TELEMETRY_HTTP_TIMEOUT_MS,
        .disable_auto_redirect = true,
    };

    s_client = esp_http_client_init(&config);
    if ( s_client == NULL )
    {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_http_client_set_header(s_client, "Content-Type", "application/json");
    if ( err != ESP_OK )
    {
        esp_http_client_cleanup(s_client);
        s_client = NULL;
        return err;
    }
    err = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "telemetry", &s_pm_lock);
    if ( err != ESP_OK )
    {
        esp_http_client_cleanup(s_client);
        s_client = NULL;
    }
    return err;
}

void env_telemetory_run(void *arg)
{
    env_telemetory_args_t *args         = (env_telemetory_args_t *)arg;
    bool                   pm_lock_held = false;

    for ( ;; )
    {
        env_measurement_t sample;
        if ( xQueueReceive(args->measurement_queue, &sample, portMAX_DELAY) != pdPASS )
        {
            continue;
        }
        if ( !pm_lock_held )
        {
            ESP_ERROR_CHECK(esp_pm_lock_acquire(s_pm_lock));
            pm_lock_held = true;
        }
        esp_err_t err = wifi_service_start(args->network_event_group);
        if ( err == ESP_OK )
        {
            EventBits_t observed =
                xEventGroupWaitBits(args->network_event_group, WIFI_SERVICE_IPV4_READY, pdFALSE, pdTRUE, pdMS_TO_TICKS(ENV_TELEMETRY_WAIT_MS));
            if ( (observed & WIFI_SERVICE_IPV4_READY) != 0 )
            {
                env_telemetory_send(&sample);
            }
            else
            {
                ESP_LOGW(TAG, "Wi-Fi connection timed out; discarding sample");
            }
        }
        else
        {
            ESP_LOGW(TAG, "Wi-Fi start failed: %s", esp_err_to_name(err));
        }

        err = wifi_service_stop(args->network_event_group);
        if ( err == ESP_OK )
        {
            ESP_ERROR_CHECK(esp_pm_lock_release(s_pm_lock));
            pm_lock_held = false;
        }
        else
        {
            // Do not permit sleep before Wi-Fi shutdown has been confirmed.
            ESP_LOGE(TAG, "Wi-Fi stop failed: %s; retaining PM lock", esp_err_to_name(err));
        }
    }
}
