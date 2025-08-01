#include <math.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "servo_motor.h"
#include "dht.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mqtt_client.h"

// LED estado --> GPIO 12
// NTC --> GPIO 34
// HC-SR04
    // PIN_TRIG --> GPIO 26
    // PIN_ECHO --> GPIO 25
// Relé --> GPIO 32
// Pulsante --> GPIO 35
// DHT22: SDA -->GPIO 13
// Servo --> GPIO 33

// Definición de pines
#define LED_STATUS GPIO_NUM_12
#define BUTTON_PIN GPIO_NUM_35
#define RELAY_PIN GPIO_NUM_32
#define NTC_PIN GPIO_NUM_34
#define TRIG_PIN GPIO_NUM_26
#define ECHO_PIN GPIO_NUM_25
#define SERVO_PIN GPIO_NUM_33
#define SDA_DHT_PIN GPIO_NUM_13

// Parámetros WiFi
#define WIFI_SSID "Wokwi-GUEST" // red visible en Wokwi, sin contraseña
#define WIFI_PASSWORD "" // red visible en Wokwi, sin contraseña

// Configuración MQTT
//#define CONFIG_BROKER_URL "mqtt3.thingspeak.com" // URL del broker MQTT público
#define CONFIG_BROKER_URL "host.wokwi.internal"
#define CONFIG_BROKER_PORT 1883 // Puerto del broker MQTT
#define CONFIG_BROKER_CLIENT_ID "" // ID del cliente MQTT
#define CONFIG_BROKER_USERNAME "" // Usuario del broker MQTT (si es necesario)
#define CONFIG_BROKER_PASSWORD "" // Contraseña del broker MQTT (si es necesario)
#define TOPIC_TS "channels/3017431/publish" // Topic para publicar datos en ThingSpeak
#define TOPIC_TEMP "patio/vivero/temperatura" // Topic para publicar temperatura
#define TOPIC_HUMIDITY "patio/vivero/humedad" // Topic para publicar humedad
#define TOPIC_NIVEL "patio/vivero/nivel" // Topic para publicar nivel de agua
#define TOPIC_RIEGO "patio/vivero/riego" // Topic para controlar el riego: activa el relé

// Variables globales
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;
static bool relay_state = false;  // Variable para controlar el estado del relé
const float NTC_BETA = 3950.0; // Constante B del NTC
float humedad = 0.0, temperatura = 0.0;


// Definición de grupo de eventos WiFi
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "MQTT_prueba";

// Función para manejar eventos de WiFi
static void wifi_event_handler(void *arg, esp_event_base_t event_base, 
                              int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        printf("WiFi STA iniciado, conectando...\n");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi desconectado, reconectando...\n");
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("IP obtenida: " IPSTR "\n", IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Función de configuración de led de estado
void configure_led(void) {
    gpio_reset_pin(LED_STATUS);
    gpio_set_direction(LED_STATUS, GPIO_MODE_OUTPUT);
}

// Función de configuración de relé
void configure_relay(void) {
    gpio_reset_pin(RELAY_PIN);
    gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_PIN, 0);  // Iniciar apagado
}

// Función de visualización de estado de conexión WiFi
void led_task(void *pvParameter)
{
    while (1) {
        if (xEventGroupGetBits(wifi_event_group) & WIFI_CONNECTED_BIT) {
            gpio_set_level(LED_STATUS, 1);  // Encender LED, WIFI conectado
        } else {
            gpio_set_level(LED_STATUS, 0);  // Apagar LED, WIFI desconectado
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Función para manejar errores
// Esta función imprime un mensaje de error si el código de error es distinto de cero
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE("MQTT", "Last error %s: 0x%x", message, error_code);
    }
}


/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED - Conectado al broker MQTT");
        mqtt_connected = true;

        // AGREGAR: Suscribirse al topic de riego AL CONECTARSE
        msg_id = esp_mqtt_client_subscribe(client, TOPIC_RIEGO, 0);
        ESP_LOGI(TAG, "Suscripción al topic de riego realizada, msg_id=%d", msg_id);

        // Mensaje inicial de prueba
        msg_id = esp_mqtt_client_publish(client, TOPIC_TS, 
            "field1=45&field2=60&status=MQTTPUBLISH", 0, 0, 0);
        ESP_LOGI(TAG, "Publicación de prueba realizada, msg_id=%d", msg_id);
        

        //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        //ESP_LOGI(TAG, "Suscripción realizada, msg_id=%d", msg_id);

        //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        //ESP_LOGI(TAG, "Suscripción realizada, msg_id=%d", msg_id);

        //msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        //ESP_LOGI(TAG, "Desuscripción realizada, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = false;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        // AGREGAR: Procesar mensajes del topic de riego
        if (strncmp(event->topic, TOPIC_RIEGO, event->topic_len) == 0) {
            // Verificar el contenido del mensaje
            if (strncmp(event->data, "true", event->data_len) == 0 || 
                strncmp(event->data, "1", event->data_len) == 0 ||
                strncmp(event->data, "ON", event->data_len) == 0) {
                relay_state = true;
                ESP_LOGI(TAG, "Comando recibido: Activar riego");
            } else if (strncmp(event->data, "false", event->data_len) == 0 || 
                       strncmp(event->data, "0", event->data_len) == 0 ||
                       strncmp(event->data, "OFF", event->data_len) == 0) {
                relay_state = false;
                ESP_LOGI(TAG, "Comando recibido: Desactivar riego");
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        mqtt_connected = false;
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


// Función de inicio de MQTT
static void mqtt_app_start(void)
{
    // Construir la URI completa
    char broker_uri[256];
    snprintf(broker_uri, sizeof(broker_uri), "mqtt://%s:%d", CONFIG_BROKER_URL, CONFIG_BROKER_PORT);
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = broker_uri,
            },
        },
    };

    // Configurar credenciales solo si no están vacías
    if (strlen(CONFIG_BROKER_CLIENT_ID) > 0) {
        mqtt_cfg.credentials.client_id = CONFIG_BROKER_CLIENT_ID;
        ESP_LOGI(TAG, "Cliente MQTT configurado con client_id: %s", CONFIG_BROKER_CLIENT_ID);
    } else {
        ESP_LOGI(TAG, "Cliente MQTT sin client_id (generará uno automático)");
    }

    if (strlen(CONFIG_BROKER_USERNAME) > 0) {
        mqtt_cfg.credentials.username = CONFIG_BROKER_USERNAME;
        ESP_LOGI(TAG, "Cliente MQTT configurado con username: %s", CONFIG_BROKER_USERNAME);
    } else {
        ESP_LOGI(TAG, "Cliente MQTT sin username");
    }

    if (strlen(CONFIG_BROKER_PASSWORD) > 0) {
        mqtt_cfg.credentials.authentication.password = CONFIG_BROKER_PASSWORD;
        ESP_LOGI(TAG, "Cliente MQTT configurado con password");
    } else {
        ESP_LOGI(TAG, "Cliente MQTT sin password");
    }

#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "Cliente MQTT iniciado, conectando a %s", mqtt_cfg.broker.address.uri);
}


// *** Funciones para suscribirse y publicar en MQTT ***

// Función para suscribirse al topic de riego
static void subscribe_to_relay_topic(void)
{
    if (mqtt_client != NULL && mqtt_connected) {
        int msg_id = esp_mqtt_client_subscribe(mqtt_client, TOPIC_RIEGO, 0);
        ESP_LOGI(TAG, "Suscripción al topic de riego realizada, msg_id=%d", msg_id);
    } else {
        ESP_LOGE(TAG, "Cliente MQTT no inicializado o no conectado, no se puede suscribir al topic de riego.");
    }
}

// Función para publicar temperatura
void publish_temperature(float temperature) {
    if (mqtt_client != NULL) {
        char data[50];
        snprintf(data, sizeof(data), "%.2f", temperature);
        int msg_id = esp_mqtt_client_publish(mqtt_client, TOPIC_TEMP, data, 0, 1, 0);
        ESP_LOGI(TAG, "Temperatura publicada: %s, msg_id=%d", data, msg_id);
    } else {
        ESP_LOGE(TAG, "Cliente MQTT no inicializado, no se puede publicar temperatura.");
    }
}

// Función para publicar humedad
void publish_humidity(float humidity) {
    if (mqtt_client != NULL) {
        char data[50];
        snprintf(data, sizeof(data), "%.2f", humidity);
        int msg_id = esp_mqtt_client_publish(mqtt_client, TOPIC_HUMIDITY, data, 0, 1, 0);
        ESP_LOGI(TAG, "Humedad publicada: %s, msg_id=%d", data, msg_id);
    } else {
        ESP_LOGE(TAG, "Cliente MQTT no inicializado, no se puede publicar humedad.");
    }
}

// Función para publicar nivel de agua
void publish_water_level(float level) {
    if (mqtt_client != NULL) {
        char data[50];
        snprintf(data, sizeof(data), "%.2f", level);
        int msg_id = esp_mqtt_client_publish(mqtt_client, TOPIC_NIVEL, data, 0, 1, 0);
        ESP_LOGI(TAG, "Nivel de agua publicado: %s, msg_id=%d", data, msg_id);
    } else {
        ESP_LOGE(TAG, "Cliente MQTT no inicializado, no se puede publicar nivel de agua.");
    }
}

// Función para publicar el estado del relé
void publish_relay_status(bool state) {
    if (mqtt_client != NULL) {
        char data[10];
        snprintf(data, sizeof(data), "%s", state ? "ON" : "OFF");
        int msg_id = esp_mqtt_client_publish(mqtt_client, "patio/vivero/riego/status", data, 0, 1, 0);
        ESP_LOGI(TAG, "Estado del relé publicado: %s, msg_id=%d", data, msg_id);
    }
}

// Función para publicar datos en ThingSpeak
void publish_to_thingspeak(const char *data)
{
    if (mqtt_client != NULL) {
        int msg_id = esp_mqtt_client_publish(mqtt_client, TOPIC_TS, 
            data, 0, 0, 0);
        ESP_LOGI(TAG, "Datos publicados en ThingSpeak: %s, msg_id=%d", data, msg_id);
    } else {
        ESP_LOGE(TAG, "Cliente MQTT no inicializado, no se puede publicar en ThingSpeak.");
    }
}

// **** Funciones para leer sensores ****

// Función para leer distancia con HC-SR04
float leerDistancia() {
    // Configuración del GPIO para el sensor ultrasónico
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
    // Enviar pulso de 10us al pin TRIG
    gpio_set_level(TRIG_PIN, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(TRIG_PIN, 0);

    // Medir el tiempo que tarda el pulso en regresar
    int64_t start_time = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if (esp_timer_get_time() - start_time > 100000) { // Timeout de 100ms
            return -1; // Error en la medición
        }
    }
    
    start_time = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if (esp_timer_get_time() - start_time > 100000) { // Timeout de 100ms
            return -1; // Error en la medición
        }
    }
    
    int64_t end_time = esp_timer_get_time();
    float duration = (end_time - start_time) / 1000000.0; // Convertir a segundos

    // Calcular distancia en cm (34300 cm/s es la velocidad del sonido)
    return (duration * 34300.0) / 2.0; // Dividir por 2 porque es ida y vuelta
}

// Modificar activate_relay para publicar el estado
void activate_relay(bool state) {
    static bool last_state = false;  // Para evitar cambios innecesarios
    
    if (state != last_state) {
        if (state) {
            gpio_set_level(RELAY_PIN, 1); // Encender relé
            ESP_LOGI(TAG, "Relé activado");
        } else {
            gpio_set_level(RELAY_PIN, 0); // Apagar relé
            ESP_LOGI(TAG, "Relé desactivado");
        }
        
        // Publicar el estado del relé
        publish_relay_status(state);
        last_state = state;
    }
}

// Configuración del DHT22
void read_dht22(void)
{
    dht_sensor_type_t sensor_type = DHT_TYPE_AM2301; // DHT22
    gpio_num_t dht_gpio = SDA_DHT_PIN;
    gpio_pullup_en(dht_gpio);

    // Leer datos del sensor DHT22
    esp_err_t result = dht_read_float_data(sensor_type, dht_gpio, &humedad, &temperatura);
    if (result == ESP_OK)
    {
        ESP_LOGI("DHT22", "Temperatura: %.1f °C, Humedad: %.1f %%", temperatura, humedad);
        // Publicar los datos leídos
        publish_temperature(temperatura);
        publish_humidity(humedad);
    }
    else
    {
        ESP_LOGE("DHT22", "Error al leer el sensor DHT22: %s", esp_err_to_name(result));
    }
}

// Función para leer sensores y publicar datos
void sensor_task(void *pvParameter)
{
    while (1) {
        // Verificar que MQTT esté conectado antes de publicar
        if (mqtt_client != NULL && mqtt_connected) {
            float temperature = 25.0 + (rand() % 100) / 10.0;
            float humidity = temperature +5.0; // Simulación de humedad
            char data[100];
            snprintf(data, sizeof(data),
                     "field1=%.2f&field2=%.2f&status=MQTTPUBLISH", temperature, humidity);

            //printf("Publicando datos: %s\n", data);
            publish_to_thingspeak(data);

            // Lectura de sensores
            // Lógica para leer otros sensores como NTC, HC-SR04, DHT22, etc.
            float distancia = leerDistancia();
            if (distancia < 0) {
                ESP_LOGE(TAG, "Error al leer distancia");
            } else {
                ESP_LOGI(TAG, "Distancia medida: %.2f cm", distancia);
            }

            //publish_temperature(temperature); // Publicar temperatura
            //publish_humidity(humidity);       // Publicar humedad

            read_dht22(); // Leer DHT22 y publicar temperatura y humedad
            publish_water_level(distancia);   // Publicar nivel de agua
            // Activar el relé si se recibe un True en el topic de riego
            subscribe_to_relay_topic(); // Suscribirse al topic de riego

            // Lógica de control
            activate_relay(relay_state);


            // Intervalo de 16 segundos para ThingSpeak
            vTaskDelay(pdMS_TO_TICKS(16000));
        } else {
            ESP_LOGW(TAG, "MQTT no conectado, esperando...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}


// Función de conexión WiFi
void wifi_init(void *pvParameter){
    esp_netif_ip_info_t ip_info;

    // Esperar por la conexión WiFi
    printf("Conectando a WiFi...\n");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
    printf("Dirección IP: " IPSTR "\n", IP2STR(&ip_info.ip));
    printf("Conexión WiFi exitosa.\n");

    // Iniciar el cliente MQTT
    ESP_LOGI(TAG, "Iniciando cliente MQTT...");
    mqtt_app_start();

    vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar un segundo para asegurar que MQTT esté listo

    // Iniciar la tarea de lectura de sensores
    ESP_LOGI(TAG, "Iniciando tarea de lectura de sensores...");
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}






// **** Función principal de la aplicación *****
void app_main(void)
{
    printf("Iniciando aplicación...\n");

    configure_led();
    configure_relay(); // Configurar el relé

    // Inicializar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializar pila TCP/IP
    esp_netif_init();

    // Crear el bucle de eventos predeterminado
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Crear la estación Wi-Fi predeterminada
    esp_netif_create_default_wifi_sta();

    // Crear el grupo de eventos para manejar eventos de Wi-Fi
    wifi_event_group = xEventGroupCreate();

    // Mostrar el estado de conexión WiFi
    xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);

    // Iniciar driver WiFi
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    // Registrar el manejador de eventos de WiFi
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL, NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                  IP_EVENT_STA_GOT_IP,
                  &wifi_event_handler,
                  NULL, NULL));
    
                  // Configurar ajustes de conexión WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    // Configurar modo STA de WiFi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    // Iniciar WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("WiFi iniciado.\n");

    // Iniciar tarea de conexión WiFi
    xTaskCreate(wifi_init, "wifi_init", 4096, NULL, 5, NULL);  // Cambiar de 2048 a 4096
    printf("Tarea de conexión WiFi iniciada, conectando a %s...\n", WIFI_SSID);



    // Iniciar tarea de lectura de sensores
    //xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 5, NULL);
}
