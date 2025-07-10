#pragma once

void sensores_init(void);
void task_lectura_sensores(void *param);
float get_ultima_temp(void);
float get_ultima_hum(void);
float get_ultima_distancia(void);
