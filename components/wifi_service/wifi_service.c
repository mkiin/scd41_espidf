#include "wifi_service.h"

#include <stddef.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "osal_event.h"

#define SSID     "Wi-Fi-302"
#define PASSWORD "33223344"

static const char *TAG = "wifi_service";

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if ( event_base == WIFI_EVENT )
    {
        switch ( event_id )
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            osal_event_clear(OSAL_EVENT_NETWORK, WIFI_SERVICE_IPV4_READY);
            ESP_LOGI(TAG, "再接続を試みます...");
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_STOP:
            osal_event_clear(OSAL_EVENT_NETWORK, WIFI_SERVICE_IPV4_READY);
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
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "IPアドレス取得: " IPSTR, IP2STR(&event->ip_info.ip));
            osal_event_set(OSAL_EVENT_NETWORK, WIFI_SERVICE_IPV4_READY);
            break;

        case IP_EVENT_STA_LOST_IP:
            osal_event_clear(OSAL_EVENT_NETWORK, WIFI_SERVICE_IPV4_READY);
            break;

        default:
            break;
        }
    }
}

esp_err_t wifi_service_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    return ESP_OK;
}

esp_err_t wifi_service_start(void)
{
    ESP_ERROR_CHECK(osal_event_clear(OSAL_EVENT_NETWORK, WIFI_SERVICE_IPV4_READY));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}
