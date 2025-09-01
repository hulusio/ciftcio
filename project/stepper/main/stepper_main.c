#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// Define the GPIO pins connected to the L298N driver
#define IN1_PIN GPIO_NUM_5
#define IN2_PIN GPIO_NUM_4
#define IN3_PIN GPIO_NUM_0
#define IN4_PIN GPIO_NUM_2

// Stepper motor parameters
#define STEPS_PER_REVOLUTION 1000
#define STEP_DELAY_MS 10 // Adjust for desired speed

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

void app_main(void) 
{
    // Initialize motor GPIOs
    motor_gpio_init();

    while (1) 
    {
        printf("Rotating clockwise...\n");
        for (int i = 0; i < STEPS_PER_REVOLUTION; i++)
         {
            motor_step(i % 4, 1);
            vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay between rotations

        printf("Rotating counter-clockwise...\n");
        for (int i = 0; i < STEPS_PER_REVOLUTION; i++) 
        {
            motor_step(i % 4, -1);
            vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay between rotations
    }
}