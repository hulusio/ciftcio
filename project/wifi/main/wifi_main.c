#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"


void app_main(void) 
{
    // Initialize motor GPIOs
   

    while (1) 
    {
        printf("hello wifi example hello...\n");
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay between rotations
    }
}