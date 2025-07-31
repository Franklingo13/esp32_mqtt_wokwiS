#include <math.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
//#include "servo_motor.h"
//#include "dht.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"

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

// Evento
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

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

// Función de conexión WiFi
void wifi_init(void *pvParameter){
    esp_netif_ip_info_t ip_info;

    // Esperar por la conexión WiFi
    printf("Conectando a WiFi...\n");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
    printf("Dirección IP: " IPSTR "\n", IP2STR(&ip_info.ip));
    printf("Conexión WiFi exitosa.\n");

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    printf("Iniciando aplicación...\n");

    configure_led();

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
    xTaskCreate(wifi_init, "wifi_init", 2048, NULL, 5, NULL);
    printf("Tarea de conexión WiFi iniciada, conectando a %s...\n", WIFI_SSID);
}
