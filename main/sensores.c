#include "sensores.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "dht.h"
#include "ultrasonic.h"
#include "wifi_mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DHT1_GPIO 25
#define DHT2_GPIO 26
#define TRIGGER_GPIO 32
#define ECHO_GPIO 33

static const char *TAG = "SENSORES";

static float temp = -1, hum = -1, distancia = -1;

float get_ultima_temp() { return temp; }

float get_ultima_hum() { return hum; }

float get_ultima_distancia() { return distancia; }

static ultrasonic_sensor_t ultrasonic_sensor = {
    .trigger_pin = TRIGGER_GPIO,
    .echo_pin = ECHO_GPIO};

static void leer_hcsr04()
{
    uint32_t d = 0;
    esp_err_t res = ultrasonic_measure_cm(&ultrasonic_sensor, 500, &d);

    if (res == ESP_OK)
    {
        distancia = (float)d;
        ESP_LOGI(TAG, "Distancia: %.2f cm", distancia);
    }
    else
    {
        ESP_LOGW(TAG, "Error leyendo HC-SR04: %s", esp_err_to_name(res));
    }
}

static void leer_dht()
{
    int16_t t1 = 0, h1 = 0;
    int16_t t2 = 0, h2 = 0;

    bool ok1 = dht_read_data(DHT_TYPE_AM2301, DHT1_GPIO, &h1, &t1) == ESP_OK;
    bool ok2 = dht_read_data(DHT_TYPE_AM2301, DHT2_GPIO, &h2, &t2) == ESP_OK;

    if (ok1 && ok2)
    {
        temp = (t1 + t2) / 2.0 / 10.0;
        hum = h1 / 10.0; // Solo usamos humedad del sensor 1
        ESP_LOGI(TAG, "T1: %.1f°C, H1: %.1f%% | T2: %.1f°C", t1 / 10.0, h1 / 10.0, t2 / 10.0);
    }
    else if (ok1)
    {
        temp = t1 / 10.0;
        hum = h1 / 10.0;
        ESP_LOGW(TAG, "Solo sensor 1 OK. T: %.1f°C, H: %.1f%%", temp, hum);
    }
    else if (ok2)
    {
        temp = t2 / 10.0;
        hum = -1; // Indicamos que no tenemos humedad válida
        ESP_LOGW(TAG, "Solo sensor 2 OK. T: %.1f°C (sin humedad)", temp);
    }
    else
    {
        ESP_LOGE(TAG, "Error leyendo ambos sensores");
    }
}

void task_lectura_sensores(void *param)
{
    ultrasonic_init(&ultrasonic_sensor);

    while (1)
    {
        leer_hcsr04();
        leer_dht();

        esp_mqtt_client_handle_t client = get_mqtt_client();

        if (client)
        {
            char buffer[128];

            snprintf(buffer, sizeof(buffer), "{\"distancia\": %.2f}", distancia);
            int msg_id1 = esp_mqtt_client_publish(client, "sensor/distancia", buffer, 0, 1, 0);
            ESP_LOGI(TAG, "Publicando distancia. msg_id=%d", msg_id1);

            snprintf(buffer, sizeof(buffer), "{\"temperatura\": %.1f, \"humedad\": %.1f}", temp, hum);
            int msg_id2 = esp_mqtt_client_publish(client, "sensor/dht22", buffer, 0, 1, 0);
            ESP_LOGI(TAG, "Publicando DHT. msg_id=%d", msg_id2);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
