#include "env_telemetory.h"

#include <string.h>

#include "cJSON.h"
#include "env_measure.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "wifi_service.h"

#define ENV_TELEMETRY_WAIT_MS         1000U
#define ENV_TELEMETRY_HTTP_TIMEOUT_MS 5000

static const char              *TAG = "env_telemetory";
static esp_http_client_handle_t s_client;

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

    esp_http_client_set_post_field(s_client, payload, (int)strlen(payload));
    esp_err_t err = esp_http_client_perform(s_client);

    if ( err == ESP_OK )
    {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %lld", esp_http_client_get_status_code(s_client), esp_http_client_get_content_length(s_client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST 失敗: %s", esp_err_to_name(err));
        esp_http_client_close(s_client);
    }
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
    return ESP_OK;
}

void env_telemetory_run(void *arg)
{
    env_telemetory_args_t *args = (env_telemetory_args_t *)arg;

    for ( ;; )
    {
        EventBits_t observed = xEventGroupWaitBits(args->network_event_group, WIFI_SERVICE_IPV4_READY, pdFALSE, pdTRUE, pdMS_TO_TICKS(ENV_TELEMETRY_WAIT_MS));
        if ( (observed & WIFI_SERVICE_IPV4_READY) == 0 )
        {
            continue;
        }

        env_measurement_t sample;
        if ( xQueueReceive(args->measurement_queue, &sample, pdMS_TO_TICKS(ENV_TELEMETRY_WAIT_MS)) != pdPASS )
        {
            continue;
        }

        env_telemetory_send(&sample);
    }
}
