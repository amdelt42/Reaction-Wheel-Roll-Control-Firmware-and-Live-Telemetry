#include "wifi.h"
#include "mqtt.h"

static const char *TAG = "WIFI";
static int s_retry = 0;

void initialize_wifi(void)
{
    ESP_LOGI(TAG, "Initializing WiFi");

    // 1) Initialize the TCP/IP stack and the event loop
    //ESP_LOGD(TAG, "Starting TCP/IP stack and event loop");
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 2) Create default WiFi station netif
    ESP_LOGD(TAG, "Creating default WiFi STA netif");
    esp_netif_create_default_wifi_sta();

    // 3) Initialize WiFi driver with default config
    ESP_LOGD(TAG, "Initializing WiFi driver");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    // 4) Register event handlers for WiFi and IP events
    ESP_LOGD(TAG, "Registering event handlers");
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // 5) Configure and start WiFi in station mode
    ESP_LOGD(TAG, "Configuring WiFi STA — SSID: %s", WIFI_SSID);
    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGD(TAG, "Starting WiFi");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGD(TAG, "Connecting to AP");
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "WiFi configuration complete");
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data)
{ //mqtt_ever_connected
    if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (mqtt_ever_connected) {
            return;
        } else if (s_retry < 5) {
            esp_wifi_connect();
            s_retry++;
            ESP_LOGI(TAG, "Retrying WiFi connection (%d/5)", s_retry);
        } else {
            ESP_LOGE(TAG, "WiFi connection failed after 5 retries");
        }
    } else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected — IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry = 0;
        initialize_mqtt();
    }
}