#include "wifi_mqtt.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#define WIFI_SSID "Depto3"
#define WIFI_PASS "MORA2019"
#define MQTT_URI "mqtt://192.168.0.106:1883"

static const char *TAG_WIFI = "WIFI";
static const char *TAG_MQTT = "MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "Conectado al broker");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG_MQTT, "Error en cliente MQTT");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG_MQTT, "Mensaje publicado. ID: %d", event->msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG_MQTT, "Desconectado del broker");
        break;
    default:
        break;
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG_WIFI, "Iniciando conexión WiFi...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG_WIFI, "WiFi desconectado, reintentando...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "Conectado al WiFi. IP: " IPSTR, IP2STR(&event->ip_info.ip));
        iniciar_mqtt();
    }
}

void iniciar_wifi()
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

// CONFIGURAR E INICIAR UN CLIENTE MQTT
void iniciar_mqtt()
{
    // DEFINIMOS LA CONFIGURACIÓN NECESARIA PARA EL CLIENTE MQTT
    esp_mqtt_client_config_t config = {
        .broker.address.uri = MQTT_URI,
        .credentials.client_id = "ESP32_d759D4",
        .session.keepalive = 60,
    };

    // INICIALIZAMOS EL CLIENTE MQTT CON LA CONFIGURACIÓN PREDEFINIDA
    mqtt_client = esp_mqtt_client_init(&config);

    // Registra un manejador de eventos (mqtt_event_handler) para el cliente MQTT.
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    // Inicia el cliente MQTT comienza la conexión al broker y activa el sistema de eventos
    esp_mqtt_client_start(mqtt_client);
}

esp_mqtt_client_handle_t get_mqtt_client()
{
    return mqtt_client;
}