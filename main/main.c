#include "wifi_mqtt.h"
#include "sensores.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"


void task_mqtt_reconnect(void *param) {
    while (1) {
        if (!get_mqtt_client()) iniciar_mqtt();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main() {
    nvs_flash_init();
    iniciar_wifi();

    xTaskCreate(&task_lectura_sensores, "task_sensores", 4096, NULL, 5, NULL);
    xTaskCreate(&task_mqtt_reconnect, "task_mqtt", 4096, NULL, 5, NULL);
}