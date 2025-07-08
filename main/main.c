#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "dht.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

// Pines sensores
#define TRIGGER_GPIO  32
#define ECHO_GPIO     33
#define DHT1_GPIO     4
#define DHT2_GPIO     5

// WiFi
#define WIFI_SSID     "TU_SSID"
#define WIFI_PASS     "TU_PASSWORD"

// MQTT
#define MQTT_URI      "mqtt://192.168.1.100"  // IP del broker (Raspberry Pi)

static const char *TAG = "APP";
esp_mqtt_client_handle_t mqtt_client = NULL;

float obtener_distancia_hcsr04() {
    gpio_set_direction(TRIGGER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_GPIO, GPIO_MODE_INPUT);

    gpio_set_level(TRIGGER_GPIO, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIGGER_GPIO, 0);

    int64_t tiempo_inicio = esp_timer_get_time();
    while(gpio_get_level(ECHO_GPIO) == 0) {
        if (esp_timer_get_time() - tiempo_inicio > 5000) {
            ESP_LOGE(TAG, "Timeout esperando echo en alto");
            return -1.0;
        }
    }

    int64_t tiempo_inicio_medicion = esp_timer_get_time();
    while(gpio_get_level(ECHO_GPIO) == 1) {
        if (esp_timer_get_time() - tiempo_inicio_medicion > 60000) {
            ESP_LOGE(TAG, "Timeout esperando echo en bajo");
            return -1.0;
        }
    }

    int64_t duracion = esp_timer_get_time() - tiempo_inicio_medicion;
    float distancia = (duracion / 2.0) / 29.1;

    return distancia;
}

void leer_dht22_dual() {
    int16_t temp1 = 0, hum1 = 0;
    int16_t temp2 = 0, hum2 = 0;

    esp_err_t res1 = dht_read_data(DHT_TYPE_AM2301, DHT1_GPIO, &hum1, &temp1);
    esp_err_t res2 = dht_read_data(DHT_TYPE_AM2301, DHT2_GPIO, &hum2, &temp2);

    if (res1 == ESP_OK && res2 == ESP_OK) {
        float temp_prom = (temp1 + temp2) / 2.0 / 10.0;
        float hum_prom = (hum1 + hum2) / 2.0 / 10.0;

        ESP_LOGI("DHT22", "Promedio -> Temperatura: %.1f°C | Humedad: %.1f%%", temp_prom, hum_prom);

        char mensaje[100];
        snprintf(mensaje, sizeof(mensaje), "{\"temperatura\": %.1f, \"humedad\": %.1f}", temp_prom, hum_prom);
        esp_mqtt_client_publish(mqtt_client, "sensor/dht22", mensaje, 0, 1, 0);
    } else {
        if (res1 != ESP_OK) ESP_LOGE("DHT22", "Error leyendo sensor 1 (GPIO %d): %d", DHT1_GPIO, res1);
        if (res2 != ESP_OK) ESP_LOGE("DHT22", "Error leyendo sensor 2 (GPIO %d): %d", DHT2_GPIO, res2);
    }
}

void wifi_init() {
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

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    if (event->event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI("MQTT", "Conectado al broker");
    } else if (event->event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW("MQTT", "Desconectado del broker");
    }
}

void mqtt_init() {
    esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = "mqtt://broker_ip"
};

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void app_main() {
    nvs_flash_init();
    wifi_init();
    mqtt_init();

    while (1) {
        float distancia = obtener_distancia_hcsr04();
        if (distancia >= 0) {
            ESP_LOGI(TAG, "Distancia medida: %.2f cm", distancia);

            char mensaje[50];
            snprintf(mensaje, sizeof(mensaje), "{\"distancia\": %.2f}", distancia);
            esp_mqtt_client_publish(mqtt_client, "sensor/distancia", mensaje, 0, 1, 0);
        } else {
            ESP_LOGE(TAG, "Error al medir la distancia");
        }

        leer_dht22_dual();

        vTaskDelay(pdMS_TO_TICKS(5000));  // Esperar 5 segundos
    }
}
