#include "wifi_mqtt.h"
#include "sensores.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"


void app_main() {
    nvs_flash_init();
    iniciar_wifi();
    iniciar_mqtt();

    xTaskCreate(&task_lectura_sensores, "task_sensores", 4096, NULL, 5, NULL);
}