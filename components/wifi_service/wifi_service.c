#include "wifi_service.h"

#include <stddef.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "sdkconfig.h"

#define WIFI_SERVICE_STOPPED (UINT32_C(1) << 1)

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    EventGroupHandle_t network_event_group = (EventGroupHandle_t)arg;
    (void)event_data;

    if ( event_base == WIFI_EVENT )
    {
        switch ( event_id )
        {
        case WIFI_EVENT_STA_DISCONNECTED:
            xEventGroupClearBits(network_event_group, WIFI_SERVICE_IPV4_READY);
            break;

        case WIFI_EVENT_STA_STOP:
            xEventGroupClearBits(network_event_group, WIFI_SERVICE_IPV4_READY);
            xEventGroupSetBits(network_event_group, WIFI_SERVICE_STOPPED);
            break;

        default:
            break;
        }
    }
    else if ( event_base == IP_EVENT )
    {
        switch ( event_id )
        {
        case IP_EVENT_STA_GOT_IP:
            xEventGroupSetBits(network_event_group, WIFI_SERVICE_IPV4_READY);
            break;

        case IP_EVENT_STA_LOST_IP:
            xEventGroupClearBits(network_event_group, WIFI_SERVICE_IPV4_READY);
            break;

        default:
            break;
        }
    }
}

esp_err_t wifi_service_init(EventGroupHandle_t network_event_group)
{
    if ( network_event_group == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_APP_WIFI_SSID,
            .password = CONFIG_APP_WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    xEventGroupClearBits(network_event_group, WIFI_SERVICE_IPV4_READY);
    xEventGroupSetBits(network_event_group, WIFI_SERVICE_STOPPED);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, network_event_group));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, network_event_group));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, wifi_event_handler, network_event_group));

    return ESP_OK;
}

esp_err_t wifi_service_start(EventGroupHandle_t network_event_group)
{
    if ( network_event_group == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }
    // A previous stop must be confirmed before starting another cycle.
    if ( (xEventGroupGetBits(network_event_group) & WIFI_SERVICE_STOPPED) == 0 )
    {
        return ESP_ERR_INVALID_STATE;
    }
    xEventGroupClearBits(network_event_group, WIFI_SERVICE_IPV4_READY | WIFI_SERVICE_STOPPED);
    esp_err_t err = esp_wifi_start();
    if ( err != ESP_OK )
    {
        xEventGroupSetBits(network_event_group, WIFI_SERVICE_STOPPED);
        return err;
    }
    // Connect in the caller task; event handlers only report state.
    return esp_wifi_connect();
}

esp_err_t wifi_service_stop(EventGroupHandle_t network_event_group)
{
    if ( network_event_group == NULL )
    {
        return ESP_ERR_INVALID_ARG;
    }
    xEventGroupClearBits(network_event_group, WIFI_SERVICE_IPV4_READY);
    // Also handles a stop event arriving after the previous call timed out.
    if ( (xEventGroupGetBits(network_event_group) & WIFI_SERVICE_STOPPED) != 0 )
    {
        return ESP_OK;
    }
    esp_err_t err = esp_wifi_stop();
    if ( err != ESP_OK )
    {
        return err;
    }
    EventBits_t bits = xEventGroupWaitBits(network_event_group, WIFI_SERVICE_STOPPED, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
    if ( (bits & WIFI_SERVICE_STOPPED) == 0 )
    {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}
