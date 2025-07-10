#pragma once

#include "mqtt_client.h"

void iniciar_wifi(void);
void iniciar_mqtt(void);
esp_mqtt_client_handle_t get_mqtt_client(void);