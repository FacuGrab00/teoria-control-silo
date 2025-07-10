#include "sensores.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "dht.h"
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

static float medir_distancia()
{
    gpio_set_direction(TRIGGER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_GPIO, GPIO_MODE_INPUT);
    gpio_set_level(TRIGGER_GPIO, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIGGER_GPIO, 0);

    int64_t t0 = esp_timer_get_time();
    while (gpio_get_level(ECHO_GPIO) == 0)
    {
        if (esp_timer_get_time() - t0 > 5000)
            return -1;
    }
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(ECHO_GPIO) == 1)
    {
        if (esp_timer_get_time() - start > 60000)
            return -1;
    }

    int64_t duration = esp_timer_get_time() - start;
    return (duration / 2.0) / 29.1;
}

static void leer_hcsr04()
{
    float d = medir_distancia();
    if (d >= 0)
    {
        distancia = d;
        ESP_LOGI(TAG, "Distancia: %.2f cm", distancia);
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
    while (1)
    {
        leer_hcsr04();
        leer_dht();

        esp_mqtt_client_handle_t client = get_mqtt_client();

        if (client)
        {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "{\"distancia\": %.2f}", distancia);
            esp_mqtt_client_publish(client, "sensor/distancia", buffer, 0, 1, 0);

            snprintf(buffer, sizeof(buffer), "{\"temperatura\": %.1f, \"humedad\": %.1f}", temp, hum);
            esp_mqtt_client_publish(client, "sensor/dht22", buffer, 0, 1, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
