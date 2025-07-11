idf_component_register(SRCS "main.c" "wifi_mqtt.c" "sensores.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash esp_wifi esp_event mqtt dht ultrasonic)