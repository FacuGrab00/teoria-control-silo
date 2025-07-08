# Proyecto ESP-IDF

## Índice
- [Librerías](#Librerías)
- [Uso de sensores](#uso-de-sensores)
- [Funciones clave](#funciones-clave)

## Librerías

## `#include <stdio.h>`

Librería estándar del lenguaje C. Se utiliza para funciones de entrada/salida, especialmente para debugging o impresión por consola.

- `printf()`: imprime texto formateado.
- `snprintf()`: escribe texto formateado en un buffer.

---

## `#include "freertos/FreeRTOS.h"`

Incluye definiciones centrales del sistema operativo en tiempo real **FreeRTOS**, que utiliza el ESP32 para manejar tareas concurrentes.

---

## `#include "freertos/task.h"`

Proporciona funciones para la creación y gestión de tareas (threads livianos) bajo FreeRTOS.

- `xTaskCreate()`: crea tareas.
- `vTaskDelay()`: suspende la ejecución de una tarea por un periodo dado.

---

## `#include "driver/gpio.h"`

Permite controlar los **pines GPIO** del ESP32.

- `gpio_set_direction()`: configura el modo (entrada/salida) de un pin.
- `gpio_set_level()`: establece el estado de un pin de salida.
- `gpio_get_level()`: lee el estado de un pin de entrada.

Usado para sensores como el **HC-SR04** y el control de señales digitales.

---

## `#include "esp_log.h"`

Módulo de logging integrado del ESP-IDF.

- `ESP_LOGI()`, `ESP_LOGE()`: permiten imprimir mensajes con etiquetas y niveles (INFO, ERROR, etc).
- Muy útil para depuración.

---

## `#include "esp_timer.h"`

Proporciona acceso a **temporizadores de alta resolución** (microsegundos).

- `esp_timer_get_time()`: devuelve el tiempo actual en microsegundos desde que se inició el sistema.
- Usado para medir tiempos de respuesta precisos, por ejemplo para calcular distancia con HC-SR04.

---

## `#include "esp_rom_sys.h"`

Incluye funciones de utilidad implementadas en la **ROM del chip ESP32**.

- `esp_rom_delay_us()`: pausa la ejecución por un número dado de microsegundos.
- Útil para generar delays muy precisos sin temporizadores externos.

---

## `#include "dht.h"`

Librería externa que permite leer sensores **DHT22** (temperatura y humedad).

- `dht_read_data()`: realiza la lectura de temperatura y humedad desde un pin GPIO.

---

## `#include "mqtt_client.h"`

Librería oficial del ESP-IDF para conectarse a brokers MQTT (como Mosquitto en Raspberry Pi).

- `esp_mqtt_client_init()`: inicializa el cliente MQTT.
- `esp_mqtt_client_publish()`: envía mensajes a un tópico.
- `esp_mqtt_client_start()`: inicia la conexión al broker.

---

## `#include "nvs_flash.h"`

Permite usar la **memoria flash no volátil (NVS)** para guardar datos entre reinicios (ej: configuración WiFi).

- `nvs_flash_init()`: inicializa el sistema de almacenamiento no volátil.
- Requerido para el stack WiFi y otras funcionalidades persistentes.

---

## `#include "esp_wifi.h"`

Proporciona todas las funciones necesarias para la **conexión WiFi** del ESP32 en modo estación (STA).

- `esp_wifi_init()`, `esp_wifi_start()`, `esp_wifi_connect()`.

---

## `#include "esp_event.h"`

Sistema de eventos del ESP-IDF. Necesario para manejar eventos asincrónicos del WiFi o MQTT.

- `esp_event_loop_create_default()`: inicializa el loop de eventos.
- Se utiliza junto con WiFi y MQTT para registrar manejadores de eventos.

---

## `#include "esp_netif.h"`

Inicializa y configura la interfaz de red del ESP32 (ya sea WiFi o Ethernet).

- `esp_netif_create_default_wifi_sta()`: configura el dispositivo como cliente WiFi (station).

---

🔧 Todas estas librerías forman parte del entorno de desarrollo **ESP-IDF** y son necesarias para implementar sensores físicos, conexión WiFi y comunicación MQTT.


## Uso de sensores

# Sensor DHT22 (AM2302) - Funcionamiento

El **DHT22** es un sensor digital que mide **temperatura** y **humedad relativa**. Utiliza un protocolo de comunicación de un solo hilo (1-wire propietario) para enviar los datos al microcontrolador.

---

## ¿Qué mide?

- **Temperatura ambiente** (°C)
- **Humedad relativa** (%)

---

## Pines del sensor

1. **VCC**: Alimentación (3.3V o 5V)
2. **DATA**: Comunicación de datos
3. **NC**: No conectado (puede no estar presente)
4. **GND**: Tierra

---

## Comunicación - Paso a paso

### 1. Señal de inicio (Start Signal)

El microcontrolador:

- Configura el pin como salida.
- Baja el pin a nivel **bajo** durante al menos **1 ms**.
- Luego lo sube a nivel **alto** por unos **20-40 µs**.
- Cambia el pin a modo **entrada** para escuchar la respuesta.

### 2. Respuesta del sensor

El DHT22 responde con:

- 80 µs a nivel **bajo**
- 80 µs a nivel **alto**

Esto indica que el sensor está listo para enviar datos.

### 3. Envío de datos

Se envían **40 bits** (5 bytes):

| Byte | Descripción                      |
|------|----------------------------------|
| 1    | Parte alta de humedad            |
| 2    | Parte baja de humedad            |
| 3    | Parte alta de temperatura        |
| 4    | Parte baja de temperatura        |
| 5    | Checksum (suma de los 4 anteriores)

### 4. Codificación de bits

Cada bit se transmite así:

- Pulso bajo fijo de **~50 µs**
- Luego:
  - Pulso alto de **26-28 µs** = bit **0**
  - Pulso alto de **~70 µs** = bit **1**

### 5. Cálculo final

- La humedad y temperatura se calculan combinando los dos bytes correspondientes.
- Si el bit 15 de temperatura está en 1, el valor es negativo.
- El checksum se usa para verificar integridad.

---

## Notas

- Intervalo recomendado de lectura: **cada 2 segundos**
- No se debe leer el sensor muy seguido, puede dar datos inválidos
- Sensor relativamente lento pero preciso

---

## Resumen visual

```plaintext
MCU inicia --> Nivel bajo (1ms) --> Nivel alto (30µs) --> Escucha
DHT responde: 80µs bajo + 80µs alto
Luego transmite 40 bits codificados por duración del pulso alto
```

---

## Características técnicas

- Rango de humedad: 0–100% RH, ±2–5% precisión
- Rango de temperatura: -40 a +80 °C, ±0.5 °C precisión
- Frecuencia de muestreo: 0.5 Hz (cada 2 segundos)


## 🧭 ¿Qué es el HC-SR04?

El **HC-SR04** es un sensor ultrasónico que mide la distancia entre él y un objeto usando **ondas de sonido de alta frecuencia (ultrasonido)**. Es muy usado en robótica, IoT y automatización.

---

## ⚙️ ¿Cómo funciona?

1. **Emisión del pulso ultrasónico**  
   El sensor tiene un pin llamado **Trigger**. Cuando recibe un pulso eléctrico de al menos **10 microsegundos**, el sensor emite una **señal ultrasónica** (a unos 40 kHz) hacia adelante.

2. **Rebote del sonido**  
   Esa señal viaja por el aire, **rebota en un objeto** y vuelve al sensor.

3. **Recepción del eco (controlada por el sensor)**  
   El sensor tiene un segundo pin, llamado **Echo**.  
   - **Este pin es gestionado automáticamente por el sensor**, no por el microcontrolador.
   - El pin **Echo se pone en alto** después de que el sensor emite el ultrasonido, para indicar que está esperando el eco.
   - Cuando el eco regresa, el sensor **pone el pin Echo en bajo**.  
   - El tiempo que el pin permanece en alto representa el viaje de ida y vuelta del sonido.

4. **Medición del tiempo**  
   El tiempo que el pin Echo permanece en alto corresponde al **tiempo que tardó el ultrasonido en ir y volver**.

5. **Cálculo de distancia**  
   Como el sonido viaja a una velocidad conocida (~343 m/s o ~29.1 μs/cm), se puede usar esta fórmula:

   ```
   Distancia (cm) = Tiempo de eco (μs) / (2 × 29.1)
   ```

   - Se divide por 2 porque el tiempo incluye ida y vuelta.
   - 29.1 μs/cm es el tiempo que tarda el sonido en recorrer un centímetro en el aire.

---

## 📌 Ejemplo

Si el tiempo medido es 1164 microsegundos:

```
Distancia = 1164 / (2 × 29.1) ≈ 20 cm
```

---

## 🧾 Resumen visual

| Paso        | Acción                                                |
|-------------|--------------------------------------------------------|
| 1. Trigger  | Se envía un pulso de 10 μs                             |
| 2. Emisión  | El sensor emite un ultrasonido                         |
| 3. Rebote   | El sonido rebota en un objeto                          |
| 4. Echo     | El pin Echo se mantiene alto hasta recibir el eco     |
| 5. Medición | Se mide el tiempo en microsegundos                     |
| 6. Cálculo  | Se calcula la distancia usando una fórmula             |


## Funciones clave

# 📏 Función `obtener_distancia_hcsr04()`

Esta función mide la distancia desde el sensor HC-SR04 hasta un objeto frente a él, usando temporización precisa de los pulsos ultrasónicos.

---

## 🔧 Paso a paso del funcionamiento

### 1. **Configurar los pines GPIO**

```c
gpio_set_direction(TRIGGER_GPIO, GPIO_MODE_OUTPUT);
gpio_set_direction(ECHO_GPIO, GPIO_MODE_INPUT);
```

- El pin **Trigger** se configura como salida.
- El pin **Echo** se configura como entrada.
- Esto permite **emitir un pulso ultrasónico** y luego **escuchar el eco** desde el sensor.

---

### 2. **Enviar el pulso de activación**

```c
gpio_set_level(TRIGGER_GPIO, 1);
esp_rom_delay_us(10);
gpio_set_level(TRIGGER_GPIO, 0);
```

- Se pone el pin Trigger en **alto durante 10 μs**, que es el requisito mínimo del HC-SR04 para iniciar una medición.
- Después se baja el pin, completando el pulso.
- Este pulso hace que el sensor emita una **ráfaga ultrasónica de 8 ciclos a 40 kHz**.
- A partir de aquí, el sensor comienza a manejar el pin `Echo`.

---

### 3. **Esperar que el pin Echo se ponga en alto**

```c
int64_t tiempo_inicio = esp_timer_get_time();
while(gpio_get_level(ECHO_GPIO) == 0) {
    if (esp_timer_get_time() - tiempo_inicio > 5000) {
        ESP_LOGE(TAG, "Timeout esperando el pin echo en alto");
        return -1.0;
    }
}
```

- Esta sección **espera que el sensor active el pin `Echo`**, lo que indica que ha comenzado la escucha del eco.
- Si después de 5 milisegundos (`5000 μs`) el pin sigue en bajo, algo falló:
  - El sensor **no recibió el pulso Trigger**.
  - El sensor **está dañado o mal conectado**.
  - El pin Echo **no está configurado correctamente**.
- Se considera un error crítico de inicio de medición y se retorna `-1.0`.


---

### 4. **Medir cuánto tiempo permanece en alto el pin Echo**

```c
int64_t tiempo_inicio_medicion = esp_timer_get_time();
while(gpio_get_level(ECHO_GPIO) == 1) {
    if (esp_timer_get_time() - tiempo_inicio_medicion > 60000) {
        ESP_LOGE(TAG, "Timeout esperando el pin echo en bajo");
        return -1.0;
    }
}
int64_t duracion = esp_timer_get_time() - tiempo_inicio_medicion;
```

- Una vez que `Echo` se pone en alto, **empieza la medición del tiempo**.
- El pin se mantiene alto **hasta que el eco del ultrasonido regresa al sensor**.
- Si el pin permanece en alto más de 60 ms:
  - Es probable que el **eco no haya sido detectado** (objeto fuera de alcance).
  - Se evita un bloqueo del sistema y se retorna `-1.0`.
- Si todo funciona bien, se mide la duración del pulso `Echo`, que representa el **tiempo de ida y vuelta del sonido**.

---

### 5. **Calcular la distancia**

```c
float distancia = (duracion / 2.0) / 29.1;
```

- Se divide por 2 para obtener **solo el tiempo de ida**.
- Luego se divide por 29.1, ya que **el sonido recorre 1 cm cada 29.1 μs en el aire**.
- Esto convierte el tiempo en microsegundos a una distancia en **centímetros**.

---

### 6. **Retornar la distancia**

```c
return distancia;
```

- Si no hubo errores, se devuelve la **distancia medida**.
- Si se produjo algún timeout, la función ya habrá retornado `-1.0` antes.

---

## 🧠 ¿Por qué se hace así?

- Se utilizan **delays precisos** con `esp_rom_delay_us()` para controlar el pulso de activación.
- Se mide el tiempo en **microsegundos** con `esp_timer_get_time()` para obtener resultados exactos.
- Se implementan **timeouts** para evitar bloqueos si el sensor falla o no detecta nada.


### ✅ Requisito del sensor

Según el fabricante:

> *“You need to supply a minimum 10 μs high-level pulse to the Trigger input to start the ranging.”*

Esto significa que:

- El sensor necesita un pulso en alto (nivel HIGH) de **al menos 10 microsegundos** para empezar una medición.
- Si el pulso es más corto, el sensor **lo ignora** y no emite ultrasonido.

### 🧠 ¿Qué hace el sensor durante ese pulso?

1. Espera a que el pulso se termine (cuando pasa de alto a bajo).
2. Entonces:
   - Introduce una pequeña espera (~750 μs).
   - Emite el pulso ultrasónico.
   - Activa el pin **Echo** para comenzar la medición.

### 🧪 ¿Qué pasa si el pulso dura más?

- El sensor también lo acepta.
- Pero **no comienza la medición hasta que el pulso termina** (flanco de bajada).

---

## 🧠 Notas técnicas

| Parámetro              | Valor      | Descripción                                    |
| ---------------------- | ---------- | ---------------------------------------------- |
| Pulso Trigger          | 10 μs      | Duración mínima para activar el sensor         |
| Timeout de espera HIGH | 5 ms       | Tiempo máximo para que el pin `ECHO` se active |
| Timeout de espera LOW  | 60 ms      | Tiempo máximo para recibir el eco              |
| Velocidad del sonido   | 29.1 μs/cm | Para calcular la distancia                     |

---

### 🧾 En resumen

| Duración del pulso Trigger | Resultado                        |
|----------------------------|-----------------------------------|
| < 10 μs                    | ❌ Ignorado, no se mide           |
| = 10 μs                    | ✅ Ideal, inicia medición         |
| > 10 μs                    | ✅ Funciona, pero no es necesario |

