#if 1
// Include necessary ESP-IDF headers
#include <stdio.h>          // Standard I/O functions (for printf, though ESP_LOG is preferred)
#include "freertos/FreeRTOS.h" // FreeRTOS core definitions
#include "freertos/task.h"    // FreeRTOS task management functions
#include "driver/gpio.h"      // GPIO driver for controlling pins
#include "esp_log.h"          // ESP-IDF logging functions

// Define the tag for logging messages. This helps categorize log output.
static const char *TAG = "HULUSIO-- BLINK_EXAMPLE";

// Define the GPIO pin connected to the built-in LED on the ESP-12E.
// For most ESP-12E modules and NodeMCU boards, the built-in LED is connected to GPIO2.
// This LED is typically active-low, meaning you set the pin LOW to turn the LED ON,
// and HIGH to turn it OFF.
#define LED_BUILTIN_ESP GPIO_NUM_2

#define LED_BUILTIN_BOARD GPIO_NUM_16

// The main application entry point. In ESP-IDF, this is typically `app_main`.
void app_main(void) {
    // Configure the LED pin as an OUTPUT.
    // This function sets the GPIO pin's direction.
    
    gpio_set_direction(LED_BUILTIN_ESP, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_BUILTIN_BOARD, GPIO_MODE_OUTPUT);
    // Use ESP_LOGI for informational messages.
    // This is the standard way to print debug/info messages in ESP-IDF.
    ESP_LOGI(TAG, "--- ESP-12E Blink Sketch Started (ESP-IDF Style) ---");
    ESP_LOGI(TAG, "Built-in LED is on GPIO: %d", LED_BUILTIN_ESP);
    ESP_LOGI(TAG, "Board LED is on GPIO: %d", LED_BUILTIN_BOARD);
    // The main loop, typically implemented using a FreeRTOS task or an infinite loop
    // within app_main for simple applications.

    // Initialize both LEDs to OFF state
    // Module LED (GPIO2) is active-low, so HIGH (1) turns it OFF
    gpio_set_level(LED_BUILTIN_ESP, 1);
    // Board LED (GPIO16) is active-high, so LOW (0) turns it OFF
    gpio_set_level(LED_BUILTIN_BOARD, 0);

    while (1) {
        // --- Turn LEDs ON ---
        // Module LED (GPIO2): Set LOW (0) to turn ON
        gpio_set_level(LED_BUILTIN_ESP, 0); 
        // Board LED (GPIO16): Set HIGH (1) to turn ON
        gpio_set_level(LED_BUILTIN_BOARD, 1);  
        ESP_LOGI(TAG, "Both LEDs ON"); 
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 1 second

        // --- Turn LEDs OFF ---
        // Module LED (GPIO2): Set HIGH (1) to turn OFF
        gpio_set_level(LED_BUILTIN_ESP, 1);  
        // Board LED (GPIO16): Set LOW (0) to turn OFF
        gpio_set_level(LED_BUILTIN_BOARD, 0);   
        ESP_LOGI(TAG, "Both LEDs OFF"); 
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 1 second
    }
}


#else 


/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"


void app_main()
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ",
            chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    for (int i = 10; i >= 0; i--) 
    {
        printf("Restarting in %d seconds...\n", i);
        printf("Blink -- hulusio\n\r");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

#endif
