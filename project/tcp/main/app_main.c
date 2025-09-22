/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

static const char *TAG = "MQTT_EXAMPLE";

// MQTT konuları
#define MQTT_TOPIC_LIGHT    "/hulusio/light"
#define MQTT_TOPIC_MOTOR    "/hulusio/motor"

// Kuyruk değişkenleri
QueueHandle_t light_queue;
QueueHandle_t motor_queue;

// MQTT Mesaj Yapısı
typedef struct {
    char topic[50];
    char data[20];
} mqtt_message_t;



// Define the GPIO pins connected to the L298N driver
#define IN1_PIN GPIO_NUM_5
#define IN2_PIN GPIO_NUM_4
#define IN3_PIN GPIO_NUM_0
#define IN4_PIN GPIO_NUM_2

// Stepper motor parameters
#define STEPS_PER_REVOLUTION 1000
#define STEP_DELAY_MS 15 // Adjust for desired speed

// Full-step sequence for a bipolar stepper motor
const int full_step_sequence[4][4] =
 {
    {1, 0, 0, 0}, // Step 1
    {0, 1, 0, 0}, // Step 2
    {0, 0, 1, 0}, // Step 3
    {0, 0, 0, 1}  // Step 4
};

// Function to initialize GPIO pins
void motor_gpio_init()
 {
    gpio_set_direction(IN1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN3_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN4_PIN, GPIO_MODE_OUTPUT);
}

// Function to perform a single step
void motor_step(int step_index, int direction)
 {
    if (direction == -1)
     {  // Counter-clockwise
        gpio_set_level(IN1_PIN, full_step_sequence[step_index][0]);
        gpio_set_level(IN2_PIN, full_step_sequence[step_index][1]);
        gpio_set_level(IN3_PIN, full_step_sequence[step_index][2]);
        gpio_set_level(IN4_PIN, full_step_sequence[step_index][3]);
    }
    else
    { // Clockwise
        gpio_set_level(IN1_PIN, full_step_sequence[3 - step_index][0]);
        gpio_set_level(IN2_PIN, full_step_sequence[3 - step_index][1]);
        gpio_set_level(IN3_PIN, full_step_sequence[3 - step_index][2]);
        gpio_set_level(IN4_PIN, full_step_sequence[3 - step_index][3]);
    }
}



// --- GÖREVLER (TASKS) ---

// Yüksek öncelikli lamba kontrol görevi
void light_control_task(void *pvParameter)
{
    mqtt_message_t message;
    while(1)
    {
        // Kuyrukta mesaj gelmesini bekler
        if (xQueueReceive(light_queue, &message, portMAX_DELAY) == pdPASS)
         {
            ESP_LOGI("LIGHT_TASK", "Received light command: %s", message.data);
            if (strcmp(message.data, "ON") == 0)
            {
                // Işığı aç
                printf("Lamba aciliyor!\n");
            }
            else if (strcmp(message.data, "OFF") == 0)
             {
                // Işığı kapat
                printf("Lamba kapaniyor!\n");
            }
        }
    }
}

// Düşük öncelikli motor kontrol görevi
void motor_control_task(void *pvParameter)
{
    mqtt_message_t message;
    while(1)
    {
        // Kuyrukta mesaj gelmesini bekler
        if (xQueueReceive(motor_queue, &message, portMAX_DELAY) == pdPASS)
        {
            ESP_LOGI("MOTOR_TASK", "Received motor command: %s", message.data);
            if (strcmp(message.data, "ON") == 0)
             {
                // Motoru 3-4 saniye çalıştır
               // printf("Motor calisiyor...\n");
               // vTaskDelay(pdMS_TO_TICKS(4000)); // 4 saniye bekleme
               // printf("Motor durdu.\n");
               printf("Rotating clockwise...\n");
                for (int i = 0; i < STEPS_PER_REVOLUTION; i++)
                {
                    motor_step(i % 4, 1);
                    vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
                }
            }
            else if (strcmp(message.data, "OFF") == 0)
            {
                // Motoru 2 saniye geri çalıştır
                printf("Rotating counter-clockwise...\n");
                for (int i = 0; i < STEPS_PER_REVOLUTION; i++)
                {
                    motor_step(i % 4, -1);
                    vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
                }
            }
        }
    }
}


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "hulusio", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC_MOTOR, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC_LIGHT, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
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

            mqtt_message_t new_message;
            strncpy(new_message.topic, event->topic, event->topic_len);
            new_message.topic[event->topic_len] = '\0';
            strncpy(new_message.data, event->data, event->data_len);
            new_message.data[event->data_len] = '\0';

            // Konuya göre mesajı doğru kuyruğa gönder
            if (strcmp(new_message.topic, MQTT_TOPIC_LIGHT) == 0)
            {
                xQueueSend(light_queue, &new_message, 0);
            }
             else if (strcmp(new_message.topic, MQTT_TOPIC_MOTOR) == 0)
            {
                xQueueSend(motor_queue, &new_message, 0);
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
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
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

        // Kuyrukları oluşturma
    light_queue = xQueueCreate(10, sizeof(mqtt_message_t));
    motor_queue = xQueueCreate(10, sizeof(mqtt_message_t));

    // Görevleri oluşturma ve öncelik atama
    // Işık görevi daha yüksek öncelikli (5)
    xTaskCreate(&light_control_task, "light_control_task", 2048, NULL, 5, NULL);
    // Motor görevi daha düşük öncelikli (4)
    xTaskCreate(&motor_control_task, "motor_control_task", 2048, NULL, 4, NULL);


    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    // Initialize motor GPIOs
    motor_gpio_init();

    mqtt_app_start();
}
