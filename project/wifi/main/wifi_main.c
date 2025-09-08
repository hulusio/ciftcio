#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "esp_netif.h"

// Wi-Fi Bilgileri - Lütfen kendi bilgilerinizi buraya girin
#define WIFI_SSID      "wlhulusio"
#define WIFI_PASSWORD  "EMVcxGg"

#if 0
// MQTT Broker Bilgileri - Lütfen telefonunuzun IP adresini girin
#define MQTT_BROKER_URI "mqtt://[PHONE_IP_ADDRESS]:1883"
#define MQTT_TOPIC     "/smarthome/lights/main"
#define GPIO_LIGHT     GPIO_NUM_2 // Işık için kullanacağınız GPIO pini (örneğin D4 pini)
#else
#define MQTT_BROKER_URI "mqtt://192.168.1.103:1883"
#define MQTT_TOPIC     "hulusio"
#define GPIO_LIGHT     GPIO_NUM_2 // Işık için kullanacağınız GPIO pini (örneğin D4 pini)

#endif
static const char *TAG = "ESP_MQTT_CLIENT";

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) 
{
    if (event_id == IP_EVENT_STA_GOT_IP) 
    {
        nt_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        printf("###########################");
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

        // You can start the MQTT app here
        // mqtt_app_start();
    }
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT bağlantısı başarılı.");
            esp_mqtt_client_subscribe(client, MQTT_TOPIC, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT bağlantısı koptu.");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Yeni MQTT mesajı alındı.");
            ESP_LOGI(TAG, "TOPIC: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA: %.*s", event->data_len, event->data);

            if (strncmp(event->data, "ON", event->data_len) == 0) {
                ESP_LOGI(TAG, "Komut: AÇIK");
                gpio_set_level(GPIO_LIGHT, 1); // GPIO pinini HIGH (Açık) yap
            } else if (strncmp(event->data, "OFF", event->data_len) == 0) {
                ESP_LOGI(TAG, "Komut: KAPALI");
                gpio_set_level(GPIO_LIGHT, 0); // GPIO pinini LOW (Kapalı) yap
            }
            break;
        case MQTT_EVENT_ERROR:
            log_error_if_nonzero("MQTT hata kodu", event->error_handle->error_type);
            break;
        default:
            ESP_LOGI(TAG, "Diğer Eventler: %d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_BROKER_URI,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, (esp_event_handler_t)mqtt_event_handler_cb, client);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    //assert(sta_netif);
    //esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    //esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL, NULL);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // NOTE: MQTT app start should be after getting IP in a real scenario
    mqtt_app_start();
}