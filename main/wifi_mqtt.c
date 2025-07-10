#include "wifi_mqtt.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#define WIFI_SSID "MiRaspberryAP"
#define WIFI_PASS "123456789"
#define MQTT_URI  "mqtt://192.168.4.1"

static const char *TAG = "WIFI_MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT conectado");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT desconectado");
            mqtt_client = NULL;
            break;
        default:
            break;
    }
}

void iniciar_wifi() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

void iniciar_mqtt() {
    esp_mqtt_client_config_t config = {
        .broker.address.uri = MQTT_URI
    };
    mqtt_client = esp_mqtt_client_init(&config);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

esp_mqtt_client_handle_t get_mqtt_client() {
    return mqtt_client;
}
