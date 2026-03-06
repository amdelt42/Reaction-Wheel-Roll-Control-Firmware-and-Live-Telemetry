#include "blink.h"


void initialize_blink(void){
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),     // Select GPIO 1
        .mode = GPIO_MODE_OUTPUT,              // Set as output
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE         // Disable interrupts
    };
    gpio_config(&io_conf);
}

static TaskHandle_t blink_task_handle = NULL;

void blink_task(void *arg)
{
    uint32_t pattern = (uint32_t)arg;  

    while (1)
    {   
        switch (pattern)
        {
        case ROCKET_STATE_IDLE:
            printf("idle led\n");
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(1900));  
            break;

        case ROCKET_STATE_ARMED:
            printf("armed led\n");
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(150));
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(1650));  
            break;

        case ROCKET_STATE_LAUNCH:
            printf("launch led\n");
            gpio_set_level(LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(2000));  
            break;

        case ROCKET_STATE_ABORT:
            printf("abort led\n");
            gpio_set_level(LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(2000));    
            break;
        }
    }
}

void set_blink_pattern(rocket_state_t state)
{
    if (blink_task_handle != NULL) {
        vTaskDelete(blink_task_handle);
        blink_task_handle = NULL;
        gpio_set_level(LED_PIN, 0);
    }
    xTaskCreate(blink_task, "blink", 2048, (void*)state, 1, &blink_task_handle);
}